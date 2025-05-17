/**
 *  This file contains definition for the ProcExcavator class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_EXCAVATOR_H_
#define _RC_PROC_EXCAVATOR_H_

#include "proc.h"

namespace rcProc {

/** Excavator effect proc
 *
 *  This effect is a shortcut for computing several signals
 *  for an excavator that would need a lot of ProcMath nodes otherwise.
 *
 *  Input signals are:
 *
 *  - SignalType::ST_EX_BUCKET
 *  - SignalType::ST_EX_DIPPER
 *  - SignalType::ST_EX_BOOM
 *  - SignalType::ST_EX_SWING
 *  - SignalType::ST_THROTTLE_RIGHT
 *  - SignalType::ST_THROTTLE_LEFT
 *
 *  Output signals are:
 *
 *  - SignalType::ST_SO_HYDRAULIC_PUMP
 *  - SignalType::ST_SO_HYDRAULIC_FLOW
 *  - SignalType::ST_SO_TRACK_RATTLE
 *  - SignalType::ST_SO_BUCKET_RATTLE
 *  TODO: RPM/load
 *
 */
class ProcExcavator: public Proc {
    private:
        rcSignals::RcSignal delayedPump;
        rcSignals::RcSignal delayedFlow;
        rcSignals::RcSignal delayedTrackRattle;

        rcSignals::RcSignal oldBucket;
        rcSignals::RcSignal oldDipper;

    public:
        ProcExcavator();

        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcExcavator&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcExcavator&);
};

} // namespace

#endif // _RC_PROC_EXCAVATOR_H_
