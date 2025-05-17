/* RC engine functions controller for Arduino ESP32.
 *
 * Dynamic audio proc.
 *
 */

#include "audio_dynamic.h"
#include "signals.h"

#include <cmath>  // for floor and abs
#include <cstdint>

using namespace rcSignals;

namespace rcAudio {

AudioDynamic::AudioDynamic() :
    sample(SampleData()),
    speedType(rcSignals::SignalType::ST_NONE),
    volumeType(rcSignals::SignalType::ST_NONE) {

    volume = {1.0f, 1.0f};
    start();
}

AudioDynamic::AudioDynamic(const SampleData& sampleVal,
                const rcSignals::SignalType& speedTypeVal,
                const rcSignals::SignalType& volumeTypeVal,
                const std::array<Volume, 2> volumeVal) :
    sample(SampleData()),
    speedType(speedTypeVal),
    volumeType(volumeTypeVal) {
    volume = volumeVal;
    start();
}


void AudioDynamic::start() {
    pos = 0;
}

void AudioDynamic::step(const rcProc::StepInfo& info) {

    // prevent division by zero later on.
    // also we don't have to do anything if the sample
    // is empty.
    if (sample.size() == 0) {
        return;
    }

    rcSignals::RcSignal dynamicVolume;
    if (volumeType == SignalType::ST_NONE) {
        dynamicVolume = RCSIGNAL_MAX;
    } else {
        dynamicVolume = info.signals->get(volumeType, RCSIGNAL_NEUTRAL);
    }

    rcSignals::RcSignal speed;
    if (speedType == SignalType::ST_NONE) {
        speed = RCSIGNAL_MAX;
    } else {
        speed = info.signals->get(speedType, RCSIGNAL_NEUTRAL);
    }

    float fVolume = dynamicVolume / 1000.0f;
    float fSpeed = 1.0f / sample.size() * speed / 1000.0f;

    for (const auto interval : info.intervals) {
        copySamples(fSpeed, interval, fVolume);
    }
}

void AudioDynamic::copySamples(float posStep,
    const rcProc::SamplesInterval& interval,
    float volume) {

    auto first = interval.first;
    auto last = interval.last;

    // -- copy audio samples
    while (first != last) {

        copySample(
           sample[std::min(
               static_cast<uint16_t>(sample.size() * pos),
               static_cast<uint16_t>(sample.size() - 1))],
           first,
           volume);

        pos += posStep;
        while (pos >= 1.0) {
            pos -= 1.0;
        }
        first++;
    }
}

} // namespace

