/**
 *  This file contains definition for the ProcMap class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_MAP_H_
#define _RC_PROC_MAP_H_

#include "proc.h"

namespace rcProc {

/** Map effect proc
 *
 *  Maps the input signal to a different range.
 *  This function can also be used for a "constant", "absolute" or "reverse" function.
 *
 *  The input range -1000, 0, 1000 is mapped to negative, zero positive.
 *  Note: Input signals out of the range (1200) will produce outputs also out of range
 *  (greater than positive).
 */
class ProcMap: public Proc {
    private:
        rcSignals::RcSignal negative;
        rcSignals::RcSignal zero;
        rcSignals::RcSignal positive;

        rcSignals::SignalType inType;
        rcSignals::SignalType outType;

    public:
        ProcMap();

        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcMap&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcMap&);
};

} // namespace

#endif // _RC_PROC_MAP_H_
