/**
 *  This file contains definition for the EngineThrottle class
 *  with the RC_Engine project.
 *
 *  @file
*/

#ifndef _RC_ENGINE_THROTTLE_H_
#define _RC_ENGINE_THROTTLE_H_

#include "proc.h"

class ProcStorage;
class EngineSimulator;

namespace rcEngine {

/** This class converts a speed input signal (representing a fraction of the
 *  maximum speed) to a throttle signal.
 *
 *  The class uses the EngineGear::energyVehicle variables.
 *
 *  Inputs used are (excluding the outputs from EngineGear):
 *
 *  - rcSignals::SignalType::ST_THROTTLE
 *  - rcSignals::SignalType::ST_SPEED
 *
 *  Outputs are
 *
 *  - rcSignals::SignalType::ST_THROTTLE (optional)
 *  - rcSignals::SignalType::ST_BRAKE (optional)
 *
 */
class EngineThrottle: public rcProc::Proc {
    private:
        /** The maximum forward speed in m/s. */
        uint16_t maxSpeedForward;

        /** The maximum rear speed in m/s. */
        uint16_t maxSpeedReverse;

        /** Used for smoothly aligning the throttle to hold a certain speed. */
        rcSignals::RcSignal lastThrottle;

    public:
        EngineThrottle();

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const EngineThrottle&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, EngineThrottle&);
};

} // namespace

#endif // _RC_ENGINE_THROTTLE_H_
