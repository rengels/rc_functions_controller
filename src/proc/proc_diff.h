/**
 *  This file contains definition for the Proc class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_DIFF_H_
#define _RC_PROC_DIFF_H_

#include "proc.h"

namespace rcProc {

/** Diff effect Proc
 *
 *  This simple proc will just return the difference between the last
 *  received signal and the current signal.
 *
 *  This proc can use the same signal for input and output (although it shouldn't)
 */
class ProcDiff: public Proc{
    private:
        /** The signal from the last step
         */
        rcSignals::RcSignal lastSignal;

        /** The input signal for the effect. */
        rcSignals::SignalType inType;

        /** The output signal. Can be the same as the input signal. */
        rcSignals::SignalType outType;

    public:
        ProcDiff();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcDiff&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcDiff&);
};

} // namespace

#endif // _RC_PROC_DIFF_H_
