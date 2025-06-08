/**
 *  This file contains definition for the ProcNeutral class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_neutral.h"
#include "signals.h"

using namespace::rcSignals;

namespace rcProc {

ProcNeutral::ProcNeutral():
    initialMs(100u),
    debounceMs(100),
    neutral(50u),
    type(SignalType::ST_THROTTLE),
    state(ProcState::START),
    timeState(0u)
{}

void ProcNeutral::start() {
    state = ProcState::START;
    timeState = 0u;
}

void ProcNeutral::step(const StepInfo& info) {

    Signals* const signals = info.signals;
    RcSignal sig = (*signals)[type];

    // -- remove neutral area
    if (sig != RCSIGNAL_INVALID) {
        if (sig < -neutral) {
            sig += neutral;
        } else if (sig < neutral) {
            sig = 0;
        } else {
            sig -= neutral;
        }
    }

    // -- advance state
    timeState += info.deltaMs;
    switch(state) {
    case ProcState::START:
        if (timeState >= initialMs) {
            timeState -= initialMs;
            state = ProcState::DEBOUNCING_OFF;
        }
        sig = RCSIGNAL_INVALID;
        break;

    case ProcState::DEBOUNCING_OFF:
        if (debounceMs == 0) {
            state = ProcState::ON;
        } else if (sig != RCSIGNAL_NEUTRAL) {
            timeState = 0;
        } else {
            timeState = 0;
            state = ProcState::DEBOUNCING_ON;
        }
        sig = RCSIGNAL_INVALID;
        break;

    case ProcState::DEBOUNCING_ON:
        if (debounceMs == 0) {
            state = ProcState::ON;
        } else if (sig != RCSIGNAL_NEUTRAL) {
            timeState = 0;
            state = ProcState::DEBOUNCING_OFF;
        } else if (timeState >= debounceMs) {
            timeState = 0;
            state = ProcState::ON;
        }

        sig = RCSIGNAL_INVALID;
        break;

    case ProcState::ON:
    default:
        ;  // nothing to do
    }

    (*signals)[type] = sig;
}

} // namespace

