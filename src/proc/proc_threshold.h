/**
 *  This file contains definition for the ProcThreshold class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_THRESHOLD_H_
#define _RC_PROC_THRESHOLD_H_

#include "proc.h"

#include <cstdint>
#include <array>

namespace rcProc {

/** Threshold effect proc
 *
 *  Reacts like a configurable schmitt-trigger with hysteresis.
 *
 */
class ProcThreshold: public Proc {
    private:

        /** Threshold that activates the output */
        rcSignals::RcSignal highThreshold;

        /** Threshold that deactivates the output */
        rcSignals::RcSignal lowThreshold;

        rcSignals::SignalType inType;
        rcSignals::SignalType outType;

        /** True if currently triggered */
        bool triggered;

    public:
        ProcThreshold();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcThreshold&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcThreshold&);
};

} // namespace

#endif // _RC_PROC_THRESHOLD_H_
