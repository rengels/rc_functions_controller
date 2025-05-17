/**
 *  This file contains definition for the ProcExcavator class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_excavator.h"
#include "signals.h"

#include <cmath>  // for abs

using namespace::rcSignals;

namespace rcProc {

ProcExcavator::ProcExcavator():
    delayedPump(RCSIGNAL_NEUTRAL),
    delayedFlow(RCSIGNAL_NEUTRAL),
    delayedTrackRattle(RCSIGNAL_NEUTRAL),
    oldBucket(RCSIGNAL_NEUTRAL),
    oldDipper(RCSIGNAL_NEUTRAL)
{}

void ProcExcavator::step(const StepInfo& info) {

    Signals* const signals = info.signals;
    const RcSignal delayValue = info.deltaMs / 3;

    // determine the input
    RcSignal bucket = signals->get(SignalType::ST_EX_BUCKET, RCSIGNAL_NEUTRAL);
    RcSignal dipper = signals->get(SignalType::ST_EX_DIPPER, RCSIGNAL_NEUTRAL);
    RcSignal boom   = signals->get(SignalType::ST_EX_BOOM, RCSIGNAL_NEUTRAL);
    RcSignal swing  = signals->get(SignalType::ST_EX_SWING, RCSIGNAL_NEUTRAL);
    RcSignal throttleRight = signals->get(SignalType::ST_THROTTLE_RIGHT, RCSIGNAL_NEUTRAL);
    RcSignal throttleLeft = signals->get(SignalType::ST_THROTTLE_LEFT, RCSIGNAL_NEUTRAL);


    // hydraulic pump
    RcSignal pump =
        abs(bucket) / 5 +
        abs(dipper) / 5 +
        // dropping the boom does not cause pump load
        ((boom < RCSIGNAL_NEUTRAL) ? (-boom / 3) : 0) +
        abs(swing) / 4;

    // hydraulic flow
    RcSignal flow =
        abs(bucket) / 5 +
        abs(dipper) / 5 +
        abs(boom)  / 3 +
        abs(swing) / 4;

    // bucket rattle is triggered if the bucket or dipper signal is changed fast.
    RcSignal buckRattle =
        abs(bucket - oldBucket) +
        abs(dipper - oldDipper);
    if (buckRattle > RCSIGNAL_MAX) {
        buckRattle = RCSIGNAL_MAX;
    }
    oldBucket = bucket;
    oldDipper = dipper;

    // track rattle
    RcSignal trackRattle =
        abs(throttleRight) / 3 +
        abs(throttleLeft) / 3;

    if (delayedFlow < flow) {
        delayedFlow -= delayValue;
    }
    if (delayedFlow > flow) {
        delayedFlow += delayValue;
    }

    if (delayedTrackRattle < trackRattle) {
        delayedTrackRattle -= delayValue;
    }
    if (delayedTrackRattle > trackRattle) {
        delayedTrackRattle += delayValue;
    }

    signals->safeSet(SignalType::ST_ENGINE_LOAD, pump);
    signals->safeSet(SignalType::ST_HYDRAULIC, delayedFlow);
    signals->safeSet(SignalType::ST_BUCKET_RATTLE, buckRattle);
    signals->safeSet(SignalType::ST_TRACK_RATTLE, delayedTrackRattle);
}


} // namespace

