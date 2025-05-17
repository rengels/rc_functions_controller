/**
 *  This file contains definition for the ProcRandom class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_random.h"
#include "signals.h"

#ifdef ARDUINO
#include <esp_random.h>
#define rand esp_random
#else
#include <cstdlib>
using namespace::std;
#endif

#include <cstdint>

using namespace::rcSignals;

namespace rcProc {

ProcRandom::ProcRandom() :
    intervalMs(1000U),
    remainingTimeMs(0U),
    lastValue(0U),
    type(rcSignals::SignalType::ST_AUX1)
{}

ProcRandom::ProcRandom(uint16_t intervalMsVal, rcSignals::SignalType typeVal) :
    intervalMs(intervalMsVal),
    remainingTimeMs(0U),
    lastValue(0U),
    type(typeVal)
{}

void ProcRandom::step(const StepInfo& info) {

    if (info.deltaMs > remainingTimeMs) {
        lastValue = (rand() % 2000) - 1000;
        remainingTimeMs = intervalMs;

    } else {
        remainingTimeMs -= info.deltaMs;
    }

    if (type != rcSignals::SignalType::ST_NONE) {
        info.signals->safeSet(type, lastValue);
    }
}

} // namespace

