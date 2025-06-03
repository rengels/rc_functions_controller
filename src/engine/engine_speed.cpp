/**
 *  This file contains definition for the Speed class
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "engine_speed.h"
#include "signals.h"

#include <cstdint>
#include <cmath>
#include <algorithm> // clamp

using namespace rcSignals;

namespace rcEngine {

Speed::Speed() :
    speedLast(0.0f),
    throttleLast(0) {
}

void Speed::start() {
    speedLast = 0.0f;
    throttleLast = 0;
}

void Speed::step(
    rcSignals::TimeMs deltaMs,
    float speedCurrent,
    rcSignals::RcSignal speedTarget,
    rcSignals::RcSignal* throttle) {

    // keep stopped
    if ((speedTarget == 0) &&
        (speedCurrent == 0)) {
        *throttle = 0;
        return;
    }

    // stop
    if ((speedTarget <= 0) &&
        (throttleLast > 0)) {
        // release throttle fast
        throttleLast = 0;
    }

    // -- calculate throttle
    // prevent division by zero later on
    if (deltaMs == 0) {
        *throttle = RCSIGNAL_INVALID;
        return;
    }

    // time to target SPEED
    float changePerMs = (speedCurrent - speedLast) / deltaMs;
    float timeToTargetMs = 10000.0f;
    if (changePerMs != 0.0f) {
        timeToTargetMs = (speedTarget - speedCurrent) / changePerMs;
    }

    bool keep = false;  // true if we should keep the throttle
    bool more = false;  // true if we need more throttle

    if (timeToTargetMs < 0.0f) {
        // SPEED goes in the wrong direction
        more = (speedCurrent < speedTarget);

    } else if (timeToTargetMs < 40.0f) {
        // stop now with changes
        keep = true;

    } else if (timeToTargetMs < 100.0f) {
        // SPEED goes in the right direction but too fast
        more = (speedCurrent > speedTarget);

    } else if (timeToTargetMs > 2000.0f) {
        // SPEED goes in the right direction but too slow
        more = (speedCurrent < speedTarget);

    } else {
        keep = true;
    }

    RcSignal throttleStep = 30;

    // increase/decrease throttle
    if (keep) {
        ; // do nothing

    } else {
        if (more) {
            throttleLast = std::min(
                static_cast<RcSignal>(throttleLast + throttleStep),
                RCSIGNAL_MAX);

        } else {
            throttleLast = std::max(
                static_cast<RcSignal>(throttleLast - throttleStep),
                static_cast<RcSignal>(-RCSIGNAL_MAX));
        }
    }

    speedLast = speedCurrent;
    *throttle = throttleLast;
}


} // namespace

