/**
 *  This file contains definition for the ProcDirection class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_DIRECTION_H_
#define _RC_PROC_DIRECTION_H_

#include "proc.h"

namespace rcProc {

/** Direction effect proc
 *
 *  This effect allows to "move" a signal.
 *  Positive input will increase the output
 *  Negative input will decrease the output
 *
 *  TODO: multiply with signal
 *  TODO: one up and one down signal
 *  TODO: allow multiple signals
 */
class ProcDirection: public Proc {
    private:
        /** Change speed of the output signal
         *
         *  Change of the signal every 10ms with full input signal.
         *  This means:
         *
         *  - 2: full left to full right in 10 s
         *  - 20: full left to full right in 10 s
         *  - 200: full left to full right in 1 s
         *
         *  Note: the full signal range is from -1000 to 1000
         */
        uint16_t speed;

        /** The current output value. */
        float currentSig;

        /** The input signal for the effect. */
        rcSignals::SignalType inType;

        /** The output signal. Can be the same as the input signal. */
        rcSignals::SignalType outType;

    public:
        ProcDirection();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcDirection&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcDirection&);
};

} // namespace

#endif // _RC_PROC_DIRECTION_H_
