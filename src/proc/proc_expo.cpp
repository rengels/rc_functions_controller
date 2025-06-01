/**
 *  This file contains definition for the ProcExpo class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_expo.h"
#include "signals.h"

using namespace::rcSignals;

namespace rcProc {

ProcExpo::ProcExpo():
    b(0.5),
    inType(SignalType::ST_THROTTLE),
    outType(SignalType::ST_THROTTLE)
{}

void ProcExpo::step(const StepInfo& info) {

    Signals* const signals = info.signals;
    RcSignal sig = (*signals)[inType];

    if (sig != RCSIGNAL_INVALID) {

        const float fIn = static_cast<float>(sig) / RCSIGNAL_MAX;
        const float fOut = b * fIn + (1 - b) * fIn * fIn * fIn;

        RcSignal out = fOut * RCSIGNAL_MAX;
        (*signals)[outType] = out;
    }
}

} // namespace

