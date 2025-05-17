/**
 *  This file contains the definition definition for the ProcMissfire class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_MISFIRE_H_
#define _RC_PROC_MISFIRE_H_

#include "proc.h"

namespace rcProc {

/** This proc is for engine misfire.
 *
 *  This effect will randomly reduce the throttle to 0 to
 *  simulate a misfire and also (optionally) create an output
 *  signal when this happens.
 *
 *  - throttle to 0 randomly (missfire)
 *
 *  - SignalType::ST_THROTTLE
 *  - SignalType::ST_SPEED
 *
 */
class ProcMisfire: public Proc {
    private:

        /** An optional output signal when the misfire is triggered.
         *
         *  Can be used to trigger a misfire sound.
         */
        rcSignals::SignalType outMisfireType;

        /** The chance that a missfire occures at throttle 100.
         *
         *  The actual chance will vary according to the actual throttle.
         */
        uint8_t misfireChance;

    public:
        ProcMisfire();

        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcMisfire&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcMisfire&);
};

} // namespace

#endif // _RC_PROC_MISFIRE_H_
