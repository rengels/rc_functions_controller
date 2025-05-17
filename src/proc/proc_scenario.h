/**
 *  This file contains definition for the ProcScenario class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_SCENARIO_H_
#define _RC_PROC_SCENARIO_H_

#include "proc.h"
#include "signals.h"

#include <cstdint>
#include <array>

namespace rcProc {

/** Scenario effect proc
 *
 *  This proc allows to cycle through up to five "scenarios", e.g. light states
 *  for up to three output signals.
 *  If more than five output signals are needed, then multiple scenarios can be used.
 *
 *  This class also implements a debounce logic, so if just debouncing is needed,
 *  this can also be used.
 */
class ProcScenario: public Proc {
    private:
        /** State variable for the debounding logic. */
        enum class DebounceState : uint8_t {
            OFF = 0U,  ///< no input signals
            NEXT,  ///< The input is > RCSIGNAL_TRUE
            PREV  ///< The input is > RCSIGNAL_TRUE
        };

        static constexpr uint8_t NUM_SCENARIOS = 5;
        static constexpr rcSignals::TimeMs DEBOUNCE_TIME = 80;

        rcSignals::SignalType inType;
        std::array<rcSignals::SignalType, NUM_SCENARIOS> outTypes1;
        std::array<rcSignals::SignalType, NUM_SCENARIOS> outTypes2;
        std::array<rcSignals::SignalType, NUM_SCENARIOS> outTypes3;

        /** Number of scenarios. (2 to NUM_SCENARIOS) */
        uint8_t numScenarios;

        /** The time that this position remained active */
        rcSignals::TimeMs inputStateTime;

        /** The validated input signal position */
        DebounceState validatedState;

        /** The current input signal position */
        DebounceState inputState;

        /** The current active scenario. 0 - (NUM_SCENARIOS - 1) */
        uint8_t scenario;

    public:
        ProcScenario();

        virtual void start();
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcScenario&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcScenario&);
};

} // namespace

#endif // _RC_PROC_SCENARIO_H_
