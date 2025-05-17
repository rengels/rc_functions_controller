/**
 *  This file contains definition for the ProcDelay class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_delay.h"
#include "signals.h"

#include <cstdint>

using namespace::rcSignals;

namespace rcProc {

ProcDelay::ProcDelay():
    delayMs(500),
    inType(rcSignals::SignalType::ST_TAIL),
    outType(rcSignals::SignalType::ST_TAIL) {
}

void ProcDelay::start() {
    for (auto& slot : inputSlots) {
        slot = RCSIGNAL_NEUTRAL;
    }
    slotTimeMs = 0u;
    nextSlotIndex = 0u;
}

void ProcDelay::step(const StepInfo& info) {

    Signals* const signals = info.signals;

    // write the delayed slot
    (*signals)[outType] = inputSlots[nextSlotIndex];

    // if enough time has passed, store the input in one of the slots
    const TimeMs slotInterval = delayMs / NUM_SLOTS;
    slotTimeMs += info.deltaMs;
    while (slotTimeMs > slotInterval) {
        inputSlots[nextSlotIndex] = (*signals)[inType];
        nextSlotIndex = (nextSlotIndex + 1) % NUM_SLOTS;
        slotTimeMs -= slotInterval;
    }
}

} // namespace

