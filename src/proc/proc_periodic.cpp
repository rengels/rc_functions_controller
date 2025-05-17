/* RC engine functions controller for Arduino ESP32.
 *
 *
 */

#include "proc_periodic.h"
#include "signals.h"

#include <cstdint>
#include <cmath>  // abs
// #include <algorithm>  // for sort and clamp

using namespace rcSignals;

namespace rcProc {

ProcPeriodic::ProcPeriodic(
                   const rcSignals::SignalType freqTypeVal,
                   const rcSignals::SignalType outTypeVal,
                   const float freqMultiplierVal,
                   const float offsetVal) :
        pos(0.0f),
        freqType(freqTypeVal),
        outType(outTypeVal),
        freqMultiplier(freqMultiplierVal),
        offset(offsetVal) {
    start();
}

void ProcPeriodic::start() {
    pos = 0.0f;
}

void ProcPeriodic::step(const rcProc::StepInfo& info) {

    auto rpm = info.signals->get(freqType, rcSignals::RCSIGNAL_NEUTRAL);

    float posStep = abs(static_cast<float>(rpm) * freqMultiplier / 1000.0f);

    pos += posStep * info.deltaMs;

    while (pos >= (1.0 + offset)) {
        pos -= 1.0;
        info.signals->safeSet(outType, RCSIGNAL_MAX);
    }
}

} // namespace

