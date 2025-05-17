/**
 *  This file contains definition for the Idle class
 *  with the RC_Engine project.
 *
 *  @file
*/

#ifndef _RC_ENGINE_IDLE_H_
#define _RC_ENGINE_IDLE_H_

#include "signals.h"

#include <cstdint>

class SimpleInStream;
class SimpleOutStream;

namespace rcEngine {

/** This class handles engine idle RPM.
 *
 *  The class will try to keep a smooth throttle to provide the minimum
 *  idle RPM.
 *
 *  The idle RPM can be set to change after cranking.
 *  Same for an additional engine load to simulate cold start behaviour.
 *
 */
class Idle {
    private:
        /** Idle motor revolutions in revolutions per minute.
         *
         *  This is the starting revolution and also the value that
         *  the engine will try to hold.
         */
        uint16_t rpmIdleStart;

        /** Idle motor revolutions in revolutions per minute.
         *
         *  This is the RPM the engine will try to hold after \ref timeStart.
         */
        uint16_t rpmIdleRunning;

        /** Additional engine load at start.
         *
         *  At a cold start some energy is wasted in the engine.
         *  This is observable through a distinct different sound, expecially
         *  with older engines.
         *
         *  The load will be reduced lineary through \ref timeStart.
         *
         *  The load is in kW.
         */
        rcSignals::RcSignal loadStart;

        /** The time between \ref rpmIdleStart and \ref rpmIdleRunning. */
        rcSignals::TimeMs timeStart;

        /** The amount the throttle is changed in order to achieve the target RPM.
         *
         */
        uint16_t throttleStep;

        /** The time since the motor was started. */
        rcSignals::TimeMs timePassed;

        /** The last RPM that we've seen*/
        float rpmLast;

        /** The last idle throttle we did applied. */
        rcSignals::RcSignal throttleLast;

    public:
        Idle();

        /** Constructor for EngineSimulation, tests and the ProcStorage */
        Idle(
            uint16_t rpmIdleStartVal,
            uint16_t rpmIdleRunningVal,
            rcSignals::RcSignal loadStartVal,
            rcSignals::TimeMs timeStartVal,
            uint16_t throttleStepVal
        );

        /** Call this function when the engine is started. */
        void start();

        /** Step the idle statemachine when the engine is running.
         *
         *  @param[in] rpm The current engine RPM.
         *  @param[in] throttle The current throttle given to the engine,
         *    not counting the idle throttle.
         *    Assumption: this throttle is positive.
         *
         *  @param[out] throttleIdle Output of the idle throttle.
         *  @param[out] loadEngine Output of the engine load in kW.
         */
        void step(
            rcSignals::TimeMs deltaMs,
            float rpm,
            rcSignals::RcSignal throttle,
            rcSignals::RcSignal* throttleIdle,
            rcSignals::RcSignal* loadEngine);

        /** Returns the rpmIdleStart for EngineSimple.
         *
         *  This is used when setting the initial engine RPM after cranking.
         */
        uint16_t getRPMStart() const {
            return rpmIdleStart;
        }

        /** Returns the rpmIdleRunning.
         *
         *  This is used in several places, e.g. when downshifting. */
        uint16_t getRPM() const {
            return rpmIdleRunning;
        }

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const Idle&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, Idle&);
};

} // namespace

#endif // _RC_ENGINE_IDLE_H_
