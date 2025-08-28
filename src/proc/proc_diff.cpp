/**
 *  This file contains definition for the ProcDiff class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_diff.h"
#include "signals.h"

#include <cstdint>

using namespace::rcSignals;

namespace rcProc {

ProcDiff::ProcDiff():
    lastSignal(rcSignals::RCSIGNAL_INVALID),
    inType(rcSignals::SignalType::ST_TAIL),
    outType(rcSignals::SignalType::ST_TAIL) {
}

void ProcDiff::start() {
    lastSignal = rcSignals::RCSIGNAL_INVALID;
}

void ProcDiff::step(const StepInfo& info) {

    if (outType == SignalType::ST_NONE) {
        return;
    }

    Signals* const signals = info.signals;
    RcSignal inSig = (*signals)[inType];

    if ((inSig != RCSIGNAL_INVALID) && (lastSignal != RCSIGNAL_INVALID)) {
        (*signals)[outType] = inSig - lastSignal;
    }

    lastSignal = inSig;
}

} // namespace

