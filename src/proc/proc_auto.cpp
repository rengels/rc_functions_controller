/**
 *  This file contains definition for the ProcAuto class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_auto.h"
#include "signals.h"

#include <cstdint>
#include <cmath>  // for abs

using namespace::rcSignals;

namespace rcProc {

ProcAuto::ProcAuto():
    timeStopped(10000),
    gearLast(0)
{}

/** Helper function that converts a boolean value to a RcSignal */
static RcSignal bToSig(bool val) {
    return val ? RCSIGNAL_MAX : RCSIGNAL_NEUTRAL;
}

void ProcAuto::step(const StepInfo& info) {

    Signals* const signals = info.signals;

    // determine the input
    RcSignal steering      = (*signals)[SignalType::ST_YAW];

    RcSignal parking  = (*signals)[SignalType::ST_PARKING_BRAKE];
    RcSignal brake    = (*signals)[SignalType::ST_BRAKE];
    RcSignal gear     = (*signals)[SignalType::ST_GEAR];
    RcSignal speed    = (*signals)[SignalType::ST_SPEED];
    RcSignal indLeft  = (*signals)[SignalType::ST_LI_INDICATOR_LEFT];
    RcSignal indRight = (*signals)[SignalType::ST_LI_INDICATOR_RIGHT];
    RcSignal hazard   = (*signals)[SignalType::ST_LI_HAZARD];

    RcSignal lowbeam  = (*signals)[SignalType::ST_LOWBEAM];

    // parking
    if (parking == RCSIGNAL_INVALID &&
        speed != RCSIGNAL_INVALID) {

        if ((abs(speed) == 0u) &&
            ((gear == 0) || (gear == RCSIGNAL_INVALID))) {
            timeStopped += info.deltaMs;
            parking = (timeStopped > DELAY_TIME_PARKING);

        } else {
            timeStopped = 0u;
            parking = false;
        }

        signals->safeSet(SignalType::ST_PARKING_BRAKE,
                         bToSig(parking));
    }

    // indicators
    // kind of the input signals
    if (indLeft == RCSIGNAL_INVALID &&
        steering != RCSIGNAL_INVALID) {
        indLeft = bToSig(steering > RCSIGNAL_TRUE);
        (*signals)[SignalType::ST_LI_INDICATOR_LEFT] = indLeft;
    }
    if (indRight == RCSIGNAL_INVALID &&
        steering != RCSIGNAL_INVALID) {
        indRight = bToSig(steering < -RCSIGNAL_TRUE);
        (*signals)[SignalType::ST_LI_INDICATOR_RIGHT] = indRight;
    }

    // output signals including hazard
    if ((indLeft != RCSIGNAL_INVALID) || (hazard != RCSIGNAL_INVALID)) {
        signals->safeSet(SignalType::ST_INDICATOR_LEFT,
            bToSig(indLeft > RCSIGNAL_TRUE ||
                   ((hazard > RCSIGNAL_TRUE) && (indRight <= RCSIGNAL_TRUE))));
    }
    if ((indRight != RCSIGNAL_INVALID) || (hazard != RCSIGNAL_INVALID)) {
        signals->safeSet(SignalType::ST_INDICATOR_RIGHT,
            bToSig(indRight > RCSIGNAL_TRUE ||
                ((hazard > RCSIGNAL_TRUE) && (indLeft <= RCSIGNAL_TRUE))));
    }

    if (gear != RCSIGNAL_INVALID) {
        signals->safeSet(SignalType::ST_REVERSING, bToSig(gear < 0));

        // shifting
        signals->safeSet(SignalType::ST_SHIFTING, bToSig(gear != gearLast));
        gearLast = gear;
    }

    // combined tail light
    RcSignal tail = RCSIGNAL_NEUTRAL;
    if (lowbeam != RCSIGNAL_INVALID) {
        tail += lowbeam / 2;
    }
    if (brake != RCSIGNAL_INVALID) {
        tail += brake / 2;
    }
    signals->safeSet(SignalType::ST_TAIL, tail);

    // tire squealing
}

} // namespace

