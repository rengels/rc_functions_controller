/**
 *  This file contains definition for the ProcAuto class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_AUTO_H_
#define _RC_PROC_AUTO_H_

#include "proc.h"

namespace rcProc {

/** Auto effect proc
 *
 *  This effect is a shortcut for creating several signals
 *  out of other input signals that would need a lot of ProcMath
 *  nodes otherwise.
 *
 *  Example: SignalType::ST_LI_INDICATOR is created out of SignalType::ST_YAW
 *
 *  Input signals are:
 *
 *  - SignalType::ST_YAW
 *  - SignalType::ST_THROTTLE
 *  - SignalType::ST_SPEED
 *  - SignalType::ST_GEAR
 *  - SignalType::ST_LI_INDICATOR_LEFT
 *  - SignalType::ST_LI_INDICATOR_RIGHT
 *  - SignalType::ST_LI_HAZARD
 *  - SignalType::ST_TRAILER_SWITCH
 *
 *  Output signals are:
 *
 *  - SignalType::ST_PARKING_BRAKE   // TODO
 *  - SignalType::ST_LI_INDICATOR_LEFT
 *  - SignalType::ST_LI_INDICATOR_RIGHT
 *  - SignalType::ST_LO_INDICATOR_LEFT
 *  - SignalType::ST_LO_INDICATOR_RIGHT
 *  - SignalType::ST_LO_REVERSING
 *  - SignalType::ST_TIRES
 *  - SignalType::ST_PARKINGBRAKE
 *  - SignalType::ST_SHIFTING
 *
 */
class ProcAuto: public Proc {
    private:
        static constexpr rcSignals::TimeMs DELAY_TIME_PARKING = 1000;  ///< the delay for the parking signal once the vehicle has stopped.

        rcSignals::TimeMs timeStopped;  ///< the time in ms that the wheel RPM has been zero.
        rcSignals::RcSignal gearLast;  ///< used for shifting signal

    public:
        ProcAuto();

        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcAuto&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcAuto&);
};

} // namespace

#endif // _RC_PROC_AUTO_H_
