/**
 *  This file contains definition for the EngineThrottle class
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "engine_throttle.h"
#include "engine_gear.h"
#include "signals.h"

#include <cmath>

using namespace rcSignals;

namespace rcEngine {

EngineThrottle::EngineThrottle() :
            maxSpeedForward(180u),
            maxSpeedReverse(60u) {
}

void EngineThrottle::start() {
    lastThrottle = RCSIGNAL_NEUTRAL;
}

void EngineThrottle::step(const rcProc::StepInfo& info) {

    Signals& signals = *(info.signals);

    auto sigSpeed = signals[SignalType::ST_SPEED];

    // minimum speed handling
    // it would be hard to get a vehicle to stop otherwise
    if (std::abs(sigSpeed) < RCSIGNAL_EPSILON) {
        sigSpeed = RCSIGNAL_NEUTRAL;
    }

    float speedMax = (sigSpeed >= 0) ? maxSpeedForward : maxSpeedReverse;
    float speedCurrent = EngineGear::getSpeed();
    float speedTarget = (speedMax * sigSpeed) / RCSIGNAL_MAX;

    // weighted difference
    float diff = (speedTarget - speedCurrent) / speedMax;
    RcSignal throttle = std::clamp(
        static_cast<RcSignal>(lastThrottle + (RCSIGNAL_MAX * diff)),
        static_cast<RcSignal>(-RCSIGNAL_MAX), RCSIGNAL_MAX);

    signals[SignalType::ST_THROTTLE] = throttle;
    lastThrottle = throttle;

    // brake signal
    auto sigBrake = signals[SignalType::ST_BRAKE];
    if (sigBrake == RCSIGNAL_INVALID) {
        if (abs(speedCurrent) > abs(speedTarget)) {
            signals[SignalType::ST_BRAKE] = (abs(speedCurrent) - abs(speedTarget));
        }
    }
}


} // namespace

