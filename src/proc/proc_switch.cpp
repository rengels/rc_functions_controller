/**
 *  This file contains definition for the ProcSwitch class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_switch.h"
#include "signals.h"

#include <algorithm> // for clamp

using namespace rcSignals;

namespace rcProc {

ProcSwitch::ProcSwitch():
    inType(SignalType::ST_AUX1),
    outTypesMomentary{SignalType::ST_NONE},
    outTypesShort{SignalType::ST_NONE},
    outTypesLong{
        SignalType::ST_FOG,
        SignalType::ST_ROOF,
        SignalType::ST_NONE,
        SignalType::ST_BEACON,
        SignalType::ST_SIDE
        }
{
    start();
}

void ProcSwitch::start() {
    posLast.invalidate();
    posDebouncedLast.invalidate();

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        signalMomentary[i] = false;
        signalShort[i] = false;
        signalLong[i] = false;
    }
}

void ProcSwitch::step(const StepInfo& info) {

    Signals* const signals = info.signals;

    // -- calculate new switch position
    PosState posCurrent = posLast;
    if ((*signals)[inType] == RCSIGNAL_INVALID) {
        posCurrent.invalidate();
    } else {
        posCurrent.pos = ((*signals)[inType] + RCSIGNAL_MAX) * NUM_CHANNELS / (RCSIGNAL_MAX * 2);
        posCurrent.pos = std::clamp(
            posCurrent.pos, static_cast<int8_t>(0), static_cast<int8_t>(NUM_CHANNELS - 1));
    }

    // -- debounce
    PosState posDebounced = posDebouncedLast;
    if (posCurrent != posLast) {
        posLast = PosState(posCurrent.pos, 0);

    } else {
        posLast.time += info.deltaMs;
        if (posLast.time > TIME_MS_DEBOUNCE) {
            posDebounced = posLast;
        }
    }

    // a new signal was debounced
    if (posDebouncedLast != posDebounced) {
        if (posDebouncedLast.isValid()) {
            signalMomentary[posDebouncedLast.pos] = false;
        }
        if (posDebounced.isValid()) {
            signalMomentary[posDebounced.pos] = true;
        }

        if (posDebouncedLast.isValid()) {
            // that was a long toggle
            if (posDebouncedLast.time > TIME_MS_TOGGLE_LONG) {
                signalLong[posDebouncedLast.pos] = !signalLong[posDebouncedLast.pos];

                // that was a short toggle
            } else if (posDebouncedLast.time > TIME_MS_TOGGLE) {
                signalShort[posDebouncedLast.pos] = !signalShort[posDebouncedLast.pos];
            }
        }

        posDebouncedLast = PosState(posDebounced.pos, posLast.time);

    } else {
        posDebouncedLast = posDebounced;
    }

    // -- output all signals
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        if ((outTypesMomentary[i] != rcSignals::SignalType::ST_NONE) &&
            signalMomentary[i]) {
            (*signals)[outTypesMomentary[i]] = RCSIGNAL_MAX;
        }
        if ((outTypesShort[i] != rcSignals::SignalType::ST_NONE) &&
            signalShort[i]) {
            (*signals)[outTypesShort[i]] = RCSIGNAL_MAX;
        }
        if ((outTypesLong[i] != rcSignals::SignalType::ST_NONE) &&
            signalLong[i]) {
            (*signals)[outTypesLong[i]] = RCSIGNAL_MAX;
        }
    }
}

} // namespace

