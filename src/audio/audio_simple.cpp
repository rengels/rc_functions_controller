/* RC engine functions controller for Arduino ESP32.
 *
 * Simple audio proc
 *
 */

#include "audio_simple.h"
#include "signals.h"
#include <cstdint>

using namespace rcSignals;

namespace rcAudio {

AudioSimple::AudioSimple() :
        sample(SampleData()),
        triggerType(rcSignals::SignalType::ST_NONE) {
    volume = {1.0f, 1.0f};
    start();
}

AudioSimple::AudioSimple(
          const SampleData& sampleVal,
          const rcSignals::SignalType typeVal,
          const std::array<Volume, 2> volumeVal):
        sample(sampleVal),
        triggerType(typeVal) {
    volume = volumeVal;
    start();
}

void AudioSimple::start() {
    triggerOld = false;
    active = false;
    pos = 0;
}

void AudioSimple::step(const rcProc::StepInfo& info) {

    // -- check if triggered
    bool triggerNew =
        info.signals->get(triggerType, rcSignals::RCSIGNAL_NEUTRAL) >
        rcSignals::RCSIGNAL_TRUE;

    if (!triggerOld && triggerNew) {
        active = true;
    }
    triggerOld = triggerNew;

    for (const auto interval : info.intervals) {
        copySamples(triggerNew, interval);
    }

    // stop if end was reached
    if (pos >= sample.size()) {
        pos = 0U;
        active = false;
    }
}

void AudioSimple::copySamples(bool /*triggerNew unused*/,
    const rcProc::SamplesInterval& interval) {

    auto first = interval.first;
    auto last = interval.last;

    // -- copy audio samples
    while (active && first != last && pos < sample.size()) {
        copySample(sample[pos], first);

        pos++;
        first++;
    }
}

} // namespace

