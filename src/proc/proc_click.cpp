/**
 *  This file contains definition for the ProcClick class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_click.h"
#include "signals.h"

#include <cstdint>

using namespace::rcSignals;

namespace rcProc {

ProcClick::ProcClick():
    inType(rcSignals::SignalType::ST_AUX1),
    outTypes{rcSignals::SignalType::ST_AUX2, rcSignals::SignalType::ST_NONE},
    clickTime(800u),
    currentClick(0u),
    pressed(false),
    currentTime(0u) {
}

void ProcClick::start() {
    currentClick = 0u;
    pressed = false;
    currentTime = 0u;
}

void ProcClick::step(const StepInfo& info) {

    Signals* const signals = info.signals;
    RcSignal inSig = (*signals)[inType];

    currentTime += info.deltaMs;

    // just pressed
    if ((inSig >= RCSIGNAL_TRUE) && (!pressed)) {
        pressed = true;
        currentTime = 0u;
        currentClick++;

    // continue pressed
    } else if ((inSig >= RCSIGNAL_TRUE) && pressed) {
        ;  // nothing to do (except update the click time)

    // continue released
    } else if ((inSig < RCSIGNAL_TRUE) && (!pressed)) {
        if ((currentClick > 0) && (currentTime > clickTime)) {
            if (currentClick < outTypes.size() + 1u) {
                auto outType = outTypes[currentClick - 1];
                if (outType != SignalType::ST_NONE) {
                    (*signals)[outType] = RCSIGNAL_MAX;
                }
            }
            currentClick = 0;
        }

    // just released
    } else if ((inSig < RCSIGNAL_TRUE) && pressed) {
        pressed = false;
    }
}

} // namespace

