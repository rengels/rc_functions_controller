/**
 *  This file contains definition for the engine/vehicle simulation classes
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "engine_brake.h"
#include "signals.h"

using namespace rcSignals;

namespace rcEngine {

EngineBrake::EngineBrake() :
            brakePower(2000000.0f), // 2MW brake power
            resistance(200.0f),
            airResistance(2.0f) {
}

RcSignal EngineBrake::getBrake(const rcProc::StepInfo& info) {

    Signals& signals = *(info.signals);

    // -- throttle from speed
    // although EngineGear can also generate a throttle signal,
    // we need it earlier for determining the brake signal.
    auto throttle = signals[SignalType::ST_THROTTLE];
    auto speedTarget = signals[SignalType::ST_SPEED];
    if ((throttle == RCSIGNAL_INVALID) &&
        (speedTarget != RCSIGNAL_INVALID)) {

        speedManager.step(info.deltaMs, relativeSpeed(), speedTarget, &throttle);
    }

    auto brake = signals[SignalType::ST_BRAKE];
    if ((brake == RCSIGNAL_INVALID) &&
        (throttle != RCSIGNAL_INVALID)) {

        if (throttle < RCSIGNAL_NEUTRAL) {
            brake  = -throttle;

        } else {
            brake = RCSIGNAL_NEUTRAL;
        }
    }

    return brake;
}

float EngineBrake::getSpeedMax() const {
    if (airResistance == 0.0f) {
        return EngineGear::getSpeedMax();
    } else {
        // equilibrium of max-power vs air resistance
        // TODO: flat resistance not considered
        float v3 = maxPower / ((1.2f / 2.0f) * airResistance);
        return std::min(
            static_cast<float>(std::pow(v3, 1.0f / 3.0f)),
            EngineGear::getSpeedMax());
    }
}

void EngineBrake::step(const rcProc::StepInfo& info) {

    Signals& signals = *(info.signals);

    float power = 0.0f;  // the power that we have to remove from the vehicle

    // -- braking
    auto brake = getBrake(info);
    signals[SignalType::ST_BRAKE] = brake; // set the calculated brake signal

    if (brake > RCSIGNAL_NEUTRAL) {
        power -= (brakePower * brake) / RCSIGNAL_MAX;
    }

    // -- air resistance
    const float speed = energyVehicle.speed(massVehicle);
    power -= 1.2f / 2.0f * airResistance * speed * speed * speed;
    power -= resistance;

    energyVehicle.add(power * static_cast<float>(info.deltaMs) / 1000.0f);

    EngineGear::step(info);
}


} // namespace

