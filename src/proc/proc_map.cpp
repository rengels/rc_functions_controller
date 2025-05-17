/**
 *  This file contains definition for the ProcMap class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_map.h"
#include "signals.h"

using namespace::rcSignals;

namespace rcProc {

ProcMap::ProcMap():
    negative(0),
    zero(500),
    positive(1000),
    inType(SignalType::ST_THROTTLE),
    outType(SignalType::ST_THROTTLE)
{}

void ProcMap::step(const StepInfo& info) {

    Signals* const signals = info.signals;
    RcSignal sig = (*signals)[inType];

    if (sig != RCSIGNAL_INVALID) {
        float fSig = static_cast<float>(sig) / RCSIGNAL_MAX;

        if (fSig < 0) {
            fSig = ((negative - zero) * (-fSig)) + zero;

        } else {
            fSig = ((positive - zero) * fSig) + zero;
        }

        RcSignal out = fSig;
        (*signals)[outType] = out;
    }
}

} // namespace

