/**
 *  This file contains definition for the engine/vehicle simulation classes
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "engine_reverse.h"
#include "signals.h"

#include <cmath>
#include <algorithm> // for sort and clamp

using namespace rcSignals;

namespace rcEngine {

static constexpr float ENERGY_EPS = 10.0f;

EngineReverse::EngineReverse() :
            drivingState(DrivingState::STOPPED_FWD),
            stoppedTimeMs(0),
            reverseDelayMs(2000) {

    fullGears.set(
        std::array<float, GearCollection::NUM_GEARS>{-1.0f, 3.0f, 2.1f, 1.5f, 1.0f, 0.8f, 0.6f, 0.0f});
}

void EngineReverse::setDrivingState(const DrivingState newState) {

    if (newState == drivingState) {
        return;  // nothing to do
    }

    if ((newState == DrivingState::BACKWARD) ||
        (newState == DrivingState::STOPPED_BCK)) {
        gears = fullGears.rearGears();
    } else {
        gears = fullGears.forwardGears();
    }

    drivingState = newState;
}

void EngineReverse::drivingStatemachine(rcSignals::RcSignal signal) {
    switch (drivingState) {
    case DrivingState::STOPPED_FWD:
        if (abs(energyVehicle.get() > ENERGY_EPS)) {
            setDrivingState(DrivingState::FORWARD);
        } else if (signal < -RCSIGNAL_EPSILON) {
            setDrivingState(DrivingState::STOPPED_BCK);
        }
        break;

    case DrivingState::STOPPED_BCK:
        if (abs(energyVehicle.get() > ENERGY_EPS)) {
            setDrivingState(DrivingState::BACKWARD);
        } else if (signal > RCSIGNAL_EPSILON) {
            setDrivingState(DrivingState::STOPPED_FWD);
        }
        break;

    case DrivingState::FORWARD:
        if ((abs(energyVehicle.get()) <= ENERGY_EPS) && (gearCurrent == 0)) {
           if ((abs(signal) <= RCSIGNAL_EPSILON) || (stoppedTimeMs > reverseDelayMs)) {
               setDrivingState(DrivingState::STOPPED_FWD);
           }
        }
        break;

    case DrivingState::BACKWARD:
        if ((abs(energyVehicle.get()) <= ENERGY_EPS) && (gearCurrent == 0)) {
           if ((abs(signal) <= RCSIGNAL_EPSILON) || (stoppedTimeMs > reverseDelayMs)) {
               setDrivingState(DrivingState::STOPPED_BCK);
           }
        }
        break;
    }

}

void EngineReverse::start() {
    gears = fullGears.forwardGears();  // setDrivingState won't set the gears if the state is unchanged.
    setDrivingState(DrivingState::STOPPED_FWD);
    EngineGear::start();
}

void EngineReverse::step(const rcProc::StepInfo& info) {

    // stopped time update
    if (abs(energyVehicle.get()) <= ENERGY_EPS) {
        stoppedTimeMs += info.deltaMs;
    } else {
        stoppedTimeMs = 0;
    }

    // -- input signals
    Signals& signals = *(info.signals);

    const RcSignal throttleOrig = signals[SignalType::ST_THROTTLE];
    const RcSignal speedOrig = signals[SignalType::ST_SPEED];
    const RcSignal gearOrig = signals[SignalType::ST_GEAR];

    // -- update statemachine
    if (gearOrig != RCSIGNAL_INVALID) {
        drivingStatemachine(gearOrig * RCSIGNAL_EPSILON);

    } else if (throttleOrig != RCSIGNAL_INVALID) {
        drivingStatemachine(throttleOrig);

    } else if (speedOrig != RCSIGNAL_INVALID) {
        drivingStatemachine(speedOrig);

    } else {
        // ensure that we stop or reverse after the vehicle stopped
        // due to "invalid" input signals.
        drivingStatemachine(RCSIGNAL_NEUTRAL);
    }

    // -- apply direction
    if ((drivingState == DrivingState::BACKWARD) ||
        (drivingState == DrivingState::STOPPED_BCK)) {

        if (throttleOrig != RCSIGNAL_INVALID) {
            signals[SignalType::ST_THROTTLE] = -throttleOrig;
        }
        if (speedOrig != RCSIGNAL_INVALID) {
            signals[SignalType::ST_SPEED] = -speedOrig;
        }
        if (gearOrig != RCSIGNAL_INVALID) {
            signals[SignalType::ST_GEAR] = -gearOrig;
        }
    }

    // for manual gears, check if gear against the driving direction
    // if yes, set it to 0 until the vehicle is stopped
    RcSignal gearCorrected = signals[SignalType::ST_GEAR];
    if (gearCorrected != RCSIGNAL_INVALID) {
        if (gearCorrected < 0) {
            signals[SignalType::ST_GEAR] = 0;
        }
    }

    EngineBrake::step(info);

    // keep the generated throttle and speed, but apply the direction
    if ((drivingState == DrivingState::BACKWARD) ||
        (drivingState == DrivingState::STOPPED_BCK)) {
        if (signals[SignalType::ST_GEAR] != RCSIGNAL_INVALID) {
            signals[SignalType::ST_GEAR] =
                -signals[SignalType::ST_GEAR];
        }
        if (signals[SignalType::ST_SPEED] != RCSIGNAL_INVALID) {
            signals[SignalType::ST_SPEED] = -signals[SignalType::ST_SPEED];
        }
    }
}


} // namespace

