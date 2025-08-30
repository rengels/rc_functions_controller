/**
 *  This file contains definition for the ProcSequence class
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "proc_sequence.h"
#include "signals.h"

#include <cstdint>

using namespace::rcSignals;

namespace rcProc {

ProcSequence::ProcSequence() :
    sequenceTimeMs(0),
    inputType(SignalType::ST_BEACON),
    outputType(SignalType::ST_BEACON1),
    onOffTimes{0, 30, 80, 30, 999, 999},
    sequenceDurationMs(540),
    onlyOnce(false) {
}

void ProcSequence::start() {
    sequenceTimeMs = 0;
}

void ProcSequence::step(const StepInfo& info) {

    auto value = (*(info.signals))[inputType];
    bool triggered = (value > RCSIGNAL_TRUE);

    if (triggered || (sequenceTimeMs > 0)) {

        bool state = false;
        rcSignals::TimeMs time = 0u;
        for (const auto& delta : onOffTimes) {
            time += delta;
            if (sequenceTimeMs < time) {
                break;
            }
            state = !state;
        }

        if (sequenceTimeMs < sequenceDurationMs) {
            sequenceTimeMs += info.deltaMs;

            (*(info.signals))[outputType] =
                (state ? RCSIGNAL_MAX : RCSIGNAL_NEUTRAL);

        } else if (!onlyOnce || !triggered) {
            sequenceTimeMs = 0;
        }
    }
}

} // namespace

