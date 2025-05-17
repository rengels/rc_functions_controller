/**
 *  This file contains definition for the Speed class
 *  with the RC_Engine project.
 *
 *  @file
*/

#ifndef _RC_ENGINE_SPEED_H_
#define _RC_ENGINE_SPEED_H_

#include "signals.h"

#include <cstdint>

class SimpleInStream;
class SimpleOutStream;

namespace rcEngine {

/** This class handles converting a speed input signal to throttle.
 *
 *  This is a closed-loop controller that will try to keep a stable
 *  speed.
 *
 */
class Speed {
    private:
        /** The last speed that we've seen. */
        float speedLast;

        /** The last idle throttle we did applied. */
        rcSignals::RcSignal throttleLast;

    public:
        Speed();

        /** Call this function when the engine is started. */
        void start();

        /** Step the idle statemachine when the engine is running.
         *
         *  @param[in] speedCurrent the current speed
         *  @param[in] speedTarget the speed from the input signal
         *  @param[out] throttle the resulting throtle
         */
        void step(
            rcSignals::TimeMs deltaMs,
            float speedCurrent,
            rcSignals::RcSignal speedTarget,
            rcSignals::RcSignal* throttle);
};

} // namespace

#endif // _RC_ENGINE_SPEED_H_
