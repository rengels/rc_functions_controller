/**
 *  This file contains definition for the engine/vehicle simulation classes
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "engine_simple.h"
#include "signals.h"
#include "curve.h"
#include "power_curves.h"

#include <cmath>
#include <algorithm> // clamp

using namespace::rcSignals;

namespace rcEngine {

EngineSimple::EngineSimple() :
            engineType(EngineType::STEAM),
            crankingTimeMs(500),
            maxPower(1400000.0f),
            rpmMax(25.0f /* around 80mph */
              / (M_PI * 1.6f) /* wheel diameter */
              * 60.0f) {

    massEngine = 5e+06;  // a synthetic value since the engine simple simulation does not actually simulate vehicle mass
}

float EngineSimple::getPower(const float rpm, float throttle) const {
    throttle = std::clamp(throttle, 0.0f, 1.0f);
    float relativeRPM = rpm / std::max(static_cast<float>(rpmMax), 1.0f);

    float positivePower;
    float negativePower = powerCurveMotorBrake.map(relativeRPM);
    switch (engineType) {
    case EngineType::ELECTRIC:
        positivePower = powerCurveElectric.map(relativeRPM);
        break;
    case EngineType::DIESEL:
        positivePower = powerCurveDiesel.map(relativeRPM);
        break;
    case EngineType::PETROL:
        positivePower = powerCurvePetrol.map(relativeRPM);
        break;
    case EngineType::PETROL_TURBO:
        positivePower = powerCurvePetrolTurbo.map(relativeRPM);
        break;
    case EngineType::STEAM:
        positivePower = powerCurveSteam.map(relativeRPM);
        negativePower = relativeRPM / 2.0f - 0.2f;
        break;
    case EngineType::TURBINE:
        positivePower = powerCurveTurbine.map(relativeRPM);
        negativePower = relativeRPM / 3.0f - 0.1f;
        break;
    default:
        positivePower = 0.0f;
    }

    float relPower = (throttle * positivePower) + ((1.0f - throttle) * negativePower);
    float absPower = relPower * maxPower;

    return absPower;
}

float EngineSimple::getRPM() const {
    return energyEngine.speed(massEngine) * 60.0f;
}

void EngineSimple::setRPM(float RPM) {
    const float v = RPM / 60.0f;
    energyEngine.set(Energy::energyFromSpeed(v, massEngine));
}

rcSignals::RcSignal EngineSimple::getIgnition(const rcSignals::Signals& signals) {
    auto ignition = signals[SignalType::ST_IGNITION];

    if (ignition == RCSIGNAL_INVALID) {
        // note: negative throttle does not start the motor (see EngineReverse)
        // note2: invalid signals don't start the engine either.
        const bool wantIgnition =
            (signals.get(SignalType::ST_THROTTLE, RCSIGNAL_NEUTRAL) >
                rcSignals::RCSIGNAL_EPSILON);
        return wantIgnition;
    }

    return ignition;
}

RcSignal EngineSimple::getThrottle(
            rcSignals::RcSignal throttle,
            const rcProc::StepInfo& info) {

    Signals& signals = *info.signals;

    // -- idle
    RcSignal throttleIdle;
    RcSignal loadIdle;
    idleManager.step(info.deltaMs, getRPM(), throttle, &throttleIdle, &loadIdle);
    throttle = std::max(throttle, throttleIdle);

    // -- engine state
    if (state != EngineState::ON) {
        throttle = 0;
    }

    // -- additional load from idle
    auto load = signals.get(SignalType::ST_ENGINE_LOAD, RCSIGNAL_NEUTRAL) * 1000.0f;
    signals[SignalType::ST_ENGINE_LOAD] = load + loadIdle;

    return throttle;
}


void EngineSimple::start() {
    Proc::start();
    energyEngine.set(0.0f);
    stepTimeMs = 0u;
    state = EngineState::OFF;
}

void EngineSimple::stepEngine(rcSignals::TimeMs deltaMs, rcSignals::RcSignal ignition) {

    stepTimeMs += deltaMs;
    switch (state) {
    case EngineState::OFF:
        if (ignition >= RCSIGNAL_TRUE) {
            state = EngineState::CRANKING;
            stepTimeMs = 0u;
        }
        break;
    case EngineState::CRANKING:
        if (ignition < RCSIGNAL_TRUE) {
            state = EngineState::OFF;
            stepTimeMs = 0u;

        } else if (stepTimeMs >= crankingTimeMs) {
            state = EngineState::ON;
            stepTimeMs = 0u;

            idleManager.start();
            setRPM(idleManager.getRPMStart());
        }
        break;
    case EngineState::ON:
        if ((ignition < RCSIGNAL_TRUE) ||
            (getRPM() <= idleManager.getRPM() / 4)) {  // too slow, we stalled it

            state = EngineState::OFF;
            stepTimeMs = 0u;
        }

        break;
    default:
        state = EngineState::OFF;
    }
}

void EngineSimple::step(const rcProc::StepInfo& info) {

    Signals& signals = *info.signals;
    auto ignition = getIgnition(signals);
    stepEngine(info.deltaMs, ignition);

    // -- get inputs
    auto throttle = signals[SignalType::ST_THROTTLE];
    throttle = getThrottle(throttle, info);

    if (throttle == RCSIGNAL_INVALID) {
        throttle = 0;
    }

    // additional engine load (the signal is in kW)
    float load = signals.get(SignalType::ST_ENGINE_LOAD, RCSIGNAL_NEUTRAL) * 1000.0f;

    // -- update energy (RPM)
    const float throttleRatio = static_cast<float>(throttle) / RCSIGNAL_MAX;
    const float power = getPower(getRPM(), throttleRatio);
    energyEngine.add(
        (power - load) * (static_cast<float>(info.deltaMs) / 1000.0f));

    // -- write outputs

    // write the modified throttle back.
    // other procs (audio) might need it
    signals[SignalType::ST_THROTTLE] = throttle;
    signals.safeSet(SignalType::ST_RPM, getRPM());
    signals.safeSet(SignalType::ST_IGNITION, ignition);

}


} // namespace

