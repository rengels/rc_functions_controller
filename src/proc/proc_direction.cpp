/**
 *  This file contains definition for the ProcDirection class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_direction.h"
#include "signals.h"
#include <algorithm> // clamp

using namespace::rcSignals;

namespace rcProc {

ProcDirection::ProcDirection():
    inType(rcSignals::SignalType::ST_EX_SWING),
    outType(rcSignals::SignalType::ST_EX_SWING) {
}

void ProcDirection::start() {
    currentSig = rcSignals::RCSIGNAL_NEUTRAL;
}

void ProcDirection::step(const StepInfo& info) {

    Signals* const signals = info.signals;

    RcSignal sig = (*signals)[inType];
    if (sig != RCSIGNAL_INVALID) {

        float fSpeed = speed / 100.0f * info.deltaMs * sig / 1000.0f;
        currentSig += fSpeed;
        currentSig = std::clamp(
            currentSig,
            static_cast<float>(-RCSIGNAL_MAX),
            static_cast<float>(RCSIGNAL_MAX));

        (*signals)[outType] = currentSig;
        return;
    }
}

} // namespace

