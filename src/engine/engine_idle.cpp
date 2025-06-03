/**
 *  This file contains definition for the Idle class
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "engine_idle.h"
#include "signals.h"

#include <cstdint>
#include <cmath>
#include <algorithm> // clamp

using namespace rcSignals;

namespace rcEngine {

Idle::Idle() :
    rpmIdleStart(1100),
    rpmIdleRunning(900),
    loadStart(5),
    timeStart(10),
    throttleStep(5),
    timePassed(0u),
    throttleLast(RCSIGNAL_MAX / 4) {
}

Idle::Idle(
    uint16_t rpmIdleStartVal,
    uint16_t rpmIdleRunningVal,
    rcSignals::RcSignal loadStartVal,
    rcSignals::TimeMs timeStartVal,
    uint16_t throttleStepVal
) :
    rpmIdleStart(rpmIdleStartVal),
    rpmIdleRunning(rpmIdleRunningVal),
    loadStart(loadStartVal),
    timeStart(timeStartVal),
    throttleStep(throttleStepVal),
    timePassed(0u),
    rpmLast(rpmIdleStart),
    throttleLast(RCSIGNAL_MAX / 4) {
}

void Idle::start() {
    timePassed = 0u;
    rpmLast = rpmIdleStart;
    throttleLast = RCSIGNAL_MAX / 4; // a nice starting point for idle throttle
}

void Idle::step(
    rcSignals::TimeMs deltaMs,
    float rpm,
    rcSignals::RcSignal throttle,
    rcSignals::RcSignal* throttleIdle,
    rcSignals::RcSignal* loadEngine) {

    // -- calculate target idle RPM
    timePassed += deltaMs;

    float factor = 1.0f;
    if (timeStart > 0) {
        factor = (static_cast<float>(timePassed) / static_cast<float>(timeStart));
    }
    factor = std::clamp(factor, 0.0f, 1.0f);

    RcSignal rpmTarget = rpmIdleStart +
        ((rpmIdleRunning - rpmIdleStart) * factor);

    // for target RPM 0 we need 0 throttle (steam engine and electric car case)
    if (rpmTarget == 0.0f) {
        *throttleIdle = 0;
        *loadEngine = loadStart * (1.0f - factor);
        return;
    }

    // -- calculate throttle
    // prevent division by zero later on
    if (deltaMs == 0) {
        *throttleIdle = throttleLast;
        *loadEngine = loadStart * (1.0f - factor);
        return;
    }

    // time to target RPM
    float changePerMs = (rpm - rpmLast) / deltaMs;
    float timeToTargetMs = 1000.0f;
    if (changePerMs != 0.0f) {
        timeToTargetMs = (rpmTarget - rpm) / changePerMs;
    }

    bool keep = false;  // true if we should keep the throttle
    bool more = false;  // true if we need more throttle

    if (timeToTargetMs < 0.0f) {
        // RPM goes in the wrong direction
        more = (rpm < rpmTarget);

    } else if (timeToTargetMs < 20.0f) {
        // RPM goes in the right direction but too fast
        more = (rpm > rpmTarget);

    } else if (timeToTargetMs > 400.0f) {
        // RPM goes in the right direction but too slow
        more = (rpm < rpmTarget);

    } else {
        keep = true;
    }

    // increase/decrease idle throttle
    if (!keep) {
        if (more) {
            throttleLast = std::min(
                static_cast<RcSignal>(throttleLast + throttleStep),
                static_cast<RcSignal>(RCSIGNAL_MAX / 2));  // don't give more than 50% throttle

        } else {
            throttleLast = std::max(
                static_cast<RcSignal>(throttleLast - throttleStep),
                RCSIGNAL_NEUTRAL);
        }
    }

    rpmLast = rpm;
    *throttleIdle = std::max(throttleLast, throttle);
    *loadEngine = loadStart * (1.0f - factor);
}


} // namespace

