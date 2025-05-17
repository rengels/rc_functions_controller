/**
 *  This file contains definition for the ProcPower class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_POWER_H_
#define _RC_PROC_POWER_H_

#include "proc.h"
#include "signals.h"

#include <cstdint>
#include <array>

namespace rcProc {

/** Power monitoring proc
 *
 *  This proc is a combination of debouncing, level checking and multiplication.
 *
 *  It debounces and level-checks the input signal (ST_VCC).
 *  If it goes below the configured levels it will multiply the throttle and
 *  speed signals with a configurable factor (to save energy).
 *
 *  It will also set signals to high, indicating the levels.
 */
class ProcPower: public Proc {
    private:
        /** State variable for the debounding logic. */
        enum class DebounceState : uint8_t {
            GOOD = 0U,  ///< vcc is good
            LOW,  ///< vcc is below the low level
            EMPTY  ///< vcc is below the empty level
        };

        static constexpr rcSignals::TimeMs DEBOUNCE_TIME = 2000;

        /** The signal that is activated if the low level is triggered. */
        rcSignals::SignalType outTypeLow;

        /** The signal that is activated if the empty level is triggered. */
        rcSignals::SignalType outTypeEmpty;

        /** The input (vcc) signal limit for the low level. */
        rcSignals::RcSignal lowLevel;

        /** The input (vcc) signal limit for the empty level. */
        rcSignals::RcSignal emptyLevel;

        /** A percentage factor that is multiplied with the speed an throttle signal,
         *  once the low level is validated.
         *
         *  Values: 0-100 (usually)
         */
        uint8_t lowPercent;

        /** A percentage factor that is multiplied with the speed an throttle signal,
         *  once the empty level is validated.
         *
         *  Values: 0-100 (usually)
         */
        uint8_t emptyPercent;

        /** The time that this state remained active */
        rcSignals::TimeMs inputStateTime;

        /** The validated input signal position */
        DebounceState validatedState;

        /** The current input signal position */
        DebounceState inputState;

    public:
        ProcPower();

        virtual void start();
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcPower&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcPower&);
};

} // namespace

#endif // _RC_PROC_POWER_H_
