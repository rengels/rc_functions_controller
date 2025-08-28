/**
 *  This file contains definition for the Proc class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_CLICK_H_
#define _RC_PROC_CLICK_H_

#include "proc.h"

#include <cstdint>
#include <array>

class ProcClickTest_Simple_Test;

namespace rcProc {

/** Click effect Proc
 *
 *  This proc will allow multiple clicks to create different signals.
 *  We are assuming that the input signals are already debounced.
 *
 *  This proc can use the same signal for input and output.
 *  This proc will overwrite valid signals.
 */
class ProcClick: public Proc{
    private:
        static const uint8_t MAX_CLICKS = 5; ///< The maximum amount of clicks we can configure

        /** The input signal for the effect. */
        rcSignals::SignalType inType;

        /** The output signals. Can be the same as the input signal. */
        std::array<rcSignals::SignalType, MAX_CLICKS> outTypes;

        /** The maximum time between clicks. */
        rcSignals::TimeMs clickTime;

        /** The number of click we are currently in */
        uint8_t currentClick;

        /** True if the input signal was active during the last step. */
        bool pressed;

        /** The time starting from the last press. */
        rcSignals::TimeMs currentTime;

    public:
        ProcClick();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcClick&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcClick&);

        friend ProcClickTest_Simple_Test;
};

} // namespace

#endif // _RC_PROC_CLICK_H_
