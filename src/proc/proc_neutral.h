/**
 *  This file contains definition for the ProcNeutral class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_NEUTRAL_H_
#define _RC_PROC_NEUTRAL_H_

#include "proc.h"

namespace rcProc {

/** Ensures a propper neutral signal.
 *
 *  This proc has three functionalities to ensure a neutral signal:
 *
 *  - enlarging the "neutral" area, that means mapping signals around neutral to 0
 *  - initial time delay until a signal is created.
 *  - debouncing for the neutral signal
 *
 */
class ProcNeutral: public Proc {
    private:
        /** State variable for the ProcIndicator state machine. */
        enum class ProcState : uint8_t {
            START = 0U,  ///< initial state
            DEBOUNCING_OFF,  ///< waiting for the signal to get to neutral
            DEBOUNCING_ON,  ///< waiting for the debounceMs time to pass
            ON  ///< everything validated
        };

        rcSignals::TimeMs initialMs;  ///< initial time before a signal is created
        rcSignals::TimeMs debounceMs;  ///< the time that the signal has to stay neutral
        uint8_t neutral;  ///< Range around 0 that is considered neutral (0) and mapped to 0

        rcSignals::SignalType type;

        ProcState state;
        rcSignals::TimeMs timeState;

    public:
        ProcNeutral();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcNeutral&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcNeutral&);
};

} // namespace

#endif // _RC_PROC_NEUTRAL_H_
