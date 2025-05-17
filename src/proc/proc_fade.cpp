/**
 *  This file contains definition for the ProcFade class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_fade.h"
#include "signals.h"

#include <cstdint>

using namespace::rcSignals;

namespace rcProc {

ProcFade::ProcFade():
    fadeIn(1000),
    fadeOut(80),
    oldValues {RCSIGNAL_NEUTRAL},
    types{SignalType::ST_INDICATOR_LEFT,
        SignalType::ST_INDICATOR_RIGHT,
        SignalType::ST_BRAKE,
        SignalType::ST_TAIL}
{}

ProcFade::ProcFade(uint16_t fadeInVal,
                   uint16_t fadeOutVal,
                   std::array<rcSignals::SignalType, NUM_CHANNELS> typesVal) :
    fadeIn(fadeInVal),
    fadeOut(fadeOutVal),
    oldValues {RCSIGNAL_NEUTRAL},
    types(typesVal)
{}

void ProcFade::step(const StepInfo& info) {

    Signals* const signals = info.signals;

    RcSignal fadeInAmount = info.deltaMs * fadeIn / 10;
    if (fadeInAmount < 1) {
        fadeInAmount = 1;
    }
    RcSignal fadeOutAmount = info.deltaMs * fadeOut / 10;
    if (fadeOutAmount < 1) {
        fadeOutAmount = 1;
    }
    for (uint8_t i = 0U; i < NUM_CHANNELS; i++) {

        RcSignal newValue = (*signals)[types[i]];
        if (newValue == RCSIGNAL_INVALID) {
            continue;
        }

        // blend up
        if (newValue > oldValues[i]) {
            oldValues[i] += fadeInAmount;
            if (newValue < oldValues[i]) {
                oldValues[i] = newValue;
            }
        }
        // blend down
        if (newValue < oldValues[i]) {
            oldValues[i] -= fadeOutAmount;
            if (newValue > oldValues[i]) {
                oldValues[i] = newValue;
            }
        }

        // write back
        // note: this proc is overwriting signals (which usually procs don't)
        //    else we wouldn't be able to blend out a valid signal.
        (*signals)[types[i]] = oldValues[i];
    }
}

} // namespace

