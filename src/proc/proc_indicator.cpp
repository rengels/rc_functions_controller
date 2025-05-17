/**
 *  This file contains definition for the ProcIndicator class
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "proc_indicator.h"
#include "signals.h"

#include <cstdint>

using namespace::rcSignals;

namespace rcProc {

ProcIndicator::ProcIndicator():
    types{rcSignals::SignalType::ST_INDICATOR_LEFT,
        rcSignals::SignalType::ST_INDICATOR_RIGHT,
        rcSignals::SignalType::ST_NONE,
        rcSignals::SignalType::ST_NONE} {
}

void ProcIndicator::start() {
    blinkCntr = 0U;
    stepTimeMs = 0U;
    state = ProcState::OFF;
    for (auto& p : participating) {
        p = false;
    }
}

/** This handles the state machine.
 *
 *  Note: we only have one state machine for all the blinking.
 */
void ProcIndicator::step(const StepInfo& info) {

    Signals* const signals = info.signals;

    bool allOff = true;
    for (uint8_t i = 0U; i < NUM_CHANNELS; i++) {
        bool active = (((*signals)[types[i]] > RCSIGNAL_TRUE) &&
                       ((*signals)[types[i]] != RCSIGNAL_INVALID));
        if (active) {
            allOff = false;
        }

        // in BLINK_ON state, just add signals. Ensure that they complete a full cycle
        // until they go black.
        if (types[i] == SignalType::ST_NONE) {
            participating[i] = false;
        } if (state == ProcState::BLINK_ON) {
            participating[i] |= active;
        } else {
            participating[i] = active;
        }
    }

    stepTimeMs += info.deltaMs;
    switch (state) {
    case ProcState::OFF:
        if (!allOff) {
            blinkCntr = 0u;
            stepTimeMs = 0u;
            state = ProcState::BLINK_ON;
        }
        break;
    case ProcState::BLINK_ON:
        if (stepTimeMs > TIME_BLINK_ON) {
            stepTimeMs -= TIME_BLINK_ON;
            state = ProcState::BLINK_OFF;
        }
        break;
    case ProcState::BLINK_OFF:
        if (allOff && (blinkCntr > 2u)) {
            stepTimeMs = 0u;
            state = ProcState::OFF;

        } else if (stepTimeMs > TIME_BLINK_OFF) {
            if (blinkCntr < 127u) {
                blinkCntr++;
            }
            stepTimeMs -= TIME_BLINK_OFF;
            state = ProcState::BLINK_ON;
        }
        break;
    default:
        state = ProcState::OFF;
    }

    // switch the signals
    for (uint8_t i = 0U; i < NUM_CHANNELS; i++) {
        if (participating[i]) {
            if (state == ProcState::BLINK_OFF) {
                (*signals)[types[i]] = RCSIGNAL_NEUTRAL;
            } else if (state == ProcState::BLINK_ON) {
                (*signals)[types[i]] = RCSIGNAL_MAX;
            }
        }
    }
}

} // namespace

