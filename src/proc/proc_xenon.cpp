/**
 *  This file contains definition for the Proc class
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "proc_xenon.h"
#include "signals.h"

using namespace rcSignals;

namespace rcProc {

ProcXenon::ProcXenon():
    stepTimeMs {0U},
    types{rcSignals::SignalType::ST_HIGHBEAM,
        rcSignals::SignalType::ST_LOWBEAM}
{}

void ProcXenon::start() {
    for (auto& state : states) {
        state = ProcState::OFF;
    }
}

void ProcXenon::step(const StepInfo& info) {

    Signals* const signals = info.signals;

    for (uint8_t i = 0U; i < NUM_CHANNELS; i++) {
        if ((*signals)[types[i]] != RCSIGNAL_INVALID) {

            stepTimeMs[i] += info.deltaMs;
            switch (states[i]) {
            case ProcState::OFF:
                if ((*signals)[types[i]] > RCSIGNAL_NEUTRAL) {
                    stepTimeMs[i] = 0U;
                    states[i] = ProcState::FLASH;
                }
                break;
            case ProcState::FLASH:
                if (stepTimeMs[i] > TIME_FLASH) {
                    stepTimeMs[i] -= TIME_FLASH;
                    states[i] = ProcState::ON;
                }
                break;
            case ProcState::ON:
                if ((*signals)[types[i]] <= RCSIGNAL_NEUTRAL) {
                    stepTimeMs[i] = 0U;
                    states[i] = ProcState::OFF;
                }
                break;
            default:
                states[i] = ProcState::OFF;
            }
        }

        // Dim the signal after a while
        if (states[i] == ProcState::ON) {
            (*signals)[types[i]] -= XENON_DIM;
        }
    }
}

} // namespace

