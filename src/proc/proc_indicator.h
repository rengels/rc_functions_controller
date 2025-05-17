/**
 *  This file contains definition for the ProcIndicator class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_INDICATOR_H_
#define _RC_PROC_INDICATOR_H_

#include "proc.h"
#include "signals.h"

#include <cstdint>
#include <array>

class ProcTest_Indicator_Test;

namespace rcProc {

/** This class provides a direction indicator blinking effect
 *  with 1.5Hz.
 *
 *  Note: a blink cycle will continue for all involved signals
 *   (that's why we have the *participating* variable)
 *
 *  Note2: all signals will blink in unison.
 *
 *  Note4: there will be at least three blinking cycles.
 *

@startuml
[*] --> OFF
OFF --> BLINK_ON : input is high
BLINK_ON --> BLINK_OFF : 666ms passed
BLINK_OFF --> BLINK_ON : 666ms passed
BLINK_OFF --> OFF : input is low and blink-counter > 2
@enduml

 */
class ProcIndicator: public Proc {
    private:
        static constexpr rcSignals::TimeMs TIME_BLINK_ON  = 666U; ///< time in ms where the light is on
        static constexpr rcSignals::TimeMs TIME_BLINK_OFF = 666U; ///< time in ms where the light is off

        /** State variable for the ProcIndicator state machine. */
        enum class ProcState : uint8_t {
            OFF = 0U,  ///< no input signals at all and the last blink has finished
            BLINK_ON,
            BLINK_OFF
        };

        static constexpr uint8_t NUM_CHANNELS = 4U;  ///< the number of channels for a single effect instance.
        std::array<rcSignals::SignalType, NUM_CHANNELS> types;

        uint8_t blinkCntr;   ///< the number of times we blinked
        rcSignals::TimeMs stepTimeMs;  ///< the time in the current step.
        ProcState state;

        std::array<bool, NUM_CHANNELS> participating;  ///< remembers which channels are involved in blinking

    public:
        ProcIndicator();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcIndicator&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcIndicator&);

        friend ProcTest_Indicator_Test;
};


} // namespace

#endif // _RC_PROC_INDICATOR_H_
