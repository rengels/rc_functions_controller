/**
 *  This file contains definition for the Proc class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_DELAY_H_
#define _RC_PROC_DELAY_H_

#include "proc.h"

#include <cstdint>
#include <array>

class ProcTest_Delay_Test;

namespace rcProc {

/** Delay effect Proc
 *
 *  This effect will delay the input by the given number of
 *  cycles.
 *
 *  To keep the memory consumption static, we use a fixed number of
 *  slots for the input buffer thus also a limited resolution for the delay.
 */
class ProcDelay: public Proc{
    private:
        static constexpr uint16_t NUM_SLOTS = 16U; ///< the number of input buffer slots

        std::array<rcSignals::RcSignal, NUM_SLOTS> inputSlots; ///< buffer for the past input signals.

        /** The time that has passed since we last wrote a slot in the input. */
        rcSignals::TimeMs slotTimeMs;

        /** The index of the slot we need to write next. */
        uint16_t nextSlotIndex;

        /** The number of milliseconds the input signal is delayed.
         */
        rcSignals::TimeMs delayMs;

        /** The input signal for the effect. */
        rcSignals::SignalType inType;

        /** The output signal. Can be the same as the input signal. */
        rcSignals::SignalType outType;

    public:
        ProcDelay();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcDelay&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcDelay&);

        friend ProcTest_Delay_Test;
};

} // namespace

#endif // _RC_PROC_DELAY_H_
