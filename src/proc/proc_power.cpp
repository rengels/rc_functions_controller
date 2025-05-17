/**
 *  This file contains definition for the ProcPower class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_power.h"
#include "signals.h"

#include <cstdint>
#include <array>
#include <algorithm>

using namespace::rcSignals;

namespace rcProc {

ProcPower::ProcPower():
    outTypeLow(SignalType::ST_NONE),
    outTypeEmpty(SignalType::ST_NONE),
    lowLevel(300u),
    emptyLevel(200u),
    lowPercent(50u),
    emptyPercent(0u),
    inputStateTime(0u),
    validatedState(DebounceState::GOOD),
    inputState(DebounceState::GOOD)
{}

void ProcPower::start() {
    inputStateTime = 0u;
    validatedState = DebounceState::GOOD;
    inputState = DebounceState::GOOD;
}

void ProcPower::step(const StepInfo& info) {

    Signals* const signals = info.signals;
    RcSignal sig = (*signals)[SignalType::ST_VCC];

    // -- debounce and switch states
    inputStateTime += info.deltaMs;
    if (sig != RCSIGNAL_INVALID) {

        DebounceState newState = DebounceState::GOOD;
        if (sig < emptyLevel) {
            newState = DebounceState::EMPTY;
        } else if (sig < lowLevel) {
            newState = DebounceState::LOW;
        }
        if (newState != inputState) {
            inputState = newState;
            inputStateTime = 0u;
        }

        // switch state
        if ((inputState != validatedState) &&
            (inputStateTime > DEBOUNCE_TIME)) {
            validatedState = inputState;
        }
    }

    // -- multiply throttle/speed
    auto throttle = (*signals)[SignalType::ST_THROTTLE];
    if (throttle != RCSIGNAL_INVALID) {
        if (validatedState == DebounceState::LOW) {
            throttle = throttle * 100u / lowPercent;
        } else if (validatedState == DebounceState::EMPTY) {
            throttle = throttle * 100u / emptyPercent;
        }
        (*signals)[SignalType::ST_THROTTLE] = throttle;
    }

    auto speed = (*signals)[SignalType::ST_SPEED];
    if (speed != RCSIGNAL_INVALID) {
        if (validatedState == DebounceState::LOW) {
            speed = speed * 100u / lowPercent;
        } else if (validatedState == DebounceState::EMPTY) {
            speed = speed * 100u / emptyPercent;
        }
        (*signals)[SignalType::ST_SPEED] = speed;
    }

    // -- send output signals
    if (validatedState == DebounceState::LOW) {
        if (outTypeLow != SignalType::ST_NONE) {
            (*signals)[outTypeLow] = RCSIGNAL_MAX;
        }

    } else if (validatedState == DebounceState::EMPTY) {
        if (outTypeLow != SignalType::ST_NONE) {
            (*signals)[outTypeLow] = RCSIGNAL_MAX;
        }
        if (outTypeEmpty != SignalType::ST_NONE) {
            (*signals)[outTypeEmpty] = RCSIGNAL_MAX;
        }
    }
}

} // namespace

