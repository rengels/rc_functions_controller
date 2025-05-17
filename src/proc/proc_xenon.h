/**
 *  This file contains definition for the ProcXenon class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_XENON_H_
#define _RC_PROC_XENON_H_

#include "proc.h"
#include "signals.h"
#include <array>

class ProcTest_Xenon_Test;

namespace rcProc {

/** This class provides a xenon effect. The lights are brighter
 *  for 50ms and then a little dimmer.
 *
 *  Each channel has it's own instance of the following state machine.

@startuml
[*] --> OFF
OFF --> FLASH : input is high
FLASH --> ON : 50ms passed
FLASH --> OFF : input is low
ON --> OFF : input is low
@enduml

 */
class ProcXenon: public Proc {
    private:
        static constexpr uint8_t NUM_CHANNELS = 4U; ///< the number of channels for a single effect instance.

        enum class ProcState : uint8_t {
            OFF,
            FLASH,
            ON
        };

        rcSignals::TimeMs TIME_FLASH = 50U;

        static constexpr rcSignals::RcSignal XENON_DIM = 100U;

        std::array<rcSignals::TimeMs, NUM_CHANNELS> stepTimeMs; ///< the time in the current step.
        std::array<ProcState, NUM_CHANNELS> states;
        std::array<rcSignals::SignalType, NUM_CHANNELS> types;

    public:
        ProcXenon();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcXenon&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcXenon&);

        friend ProcTest_Xenon_Test;
};


} // namespace

#endif // _RC_PROC_XENON_H_
