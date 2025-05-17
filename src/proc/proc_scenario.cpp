/**
 *  This file contains definition for the ProcScenario class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_scenario.h"
#include "signals.h"

#include <cstdint>
#include <array>
#include <algorithm>

using namespace::rcSignals;

namespace rcProc {

ProcScenario::ProcScenario():
    inType(SignalType::ST_AUX1),
    outTypes1 {SignalType::ST_NONE,
        SignalType::ST_NONE,
        SignalType::ST_LOWBEAM,
        SignalType::ST_HIGHBEAM,
        SignalType::ST_NONE},
    outTypes2 {SignalType::ST_NONE,
        SignalType::ST_NONE,
        SignalType::ST_SIDE,
        SignalType::ST_SIDE,
        SignalType::ST_SIDE
    },
    outTypes3 {SignalType::ST_NONE},
    numScenarios(4u),
    inputStateTime(0u),
    validatedState(DebounceState::OFF),
    inputState(DebounceState::OFF),
    scenario(0u)
{}

void ProcScenario::start() {
    inputStateTime = 0u;
    validatedState = DebounceState::OFF;
    inputState = DebounceState::OFF;
    scenario = 0u;
}

void ProcScenario::step(const StepInfo& info) {

    Signals* const signals = info.signals;
    RcSignal sig = (*signals)[inType];

    // -- sanitize numScenarios
    numScenarios = std::clamp(numScenarios, static_cast<uint8_t>(2u), NUM_SCENARIOS);

    // -- debounce and switch states
    inputStateTime += info.deltaMs;
    if (sig != RCSIGNAL_INVALID) {

        DebounceState newState = DebounceState::OFF;
        if (sig > RCSIGNAL_TRUE) {
            newState = DebounceState::NEXT;
        } else if (sig < -RCSIGNAL_TRUE) {
            newState = DebounceState::PREV;
        }
        if (newState != inputState) {
            inputState = newState;
            inputStateTime = 0u;
        }

        // switch state
        if ((inputState != validatedState) &&
            (inputStateTime > DEBOUNCE_TIME)) {
            validatedState = inputState;

            if (validatedState == DebounceState::NEXT) {
                scenario = (scenario + 1) % numScenarios;
            } else if (validatedState == DebounceState::NEXT) {
                scenario = (scenario + numScenarios - 1) % numScenarios;
            }
        }
    }

    // -- output signals
    if (outTypes1[scenario] != SignalType::ST_NONE) {
        (*signals)[outTypes1[scenario]] = RCSIGNAL_MAX;
    }
    if (outTypes2[scenario] != SignalType::ST_NONE) {
        (*signals)[outTypes2[scenario]] = RCSIGNAL_MAX;
    }
    if (outTypes3[scenario] != SignalType::ST_NONE) {
        (*signals)[outTypes3[scenario]] = RCSIGNAL_MAX;
    }
}

} // namespace

