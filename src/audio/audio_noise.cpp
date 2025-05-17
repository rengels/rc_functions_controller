/* RC engine functions controller for Arduino ESP32.
 *
 * Noise audio proc
 *
 */

#include "audio_noise.h"
#include "signals.h"

#include <cstdint>
#include <cmath>  // abs
#include <algorithm>  // for sort and clamp

using namespace rcSignals;

/** A 32 bit hash implementation from Thomas Wang that
 *  we use for the noise generation.
 */
static uint32_t hash(uint32_t a) {
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2dU;
    a = a ^ (a >> 15);
    return a;
}

namespace rcAudio {

AudioNoise::AudioNoise() :
        noiseState(0),
        lastSample(0),
        freq(500u),
        volumeType(rcSignals::SignalType::ST_NONE),
        noiseType(NoiseType::WHITE) {
    volume = {1.0f, 1.0f};
    start();
}

AudioNoise::AudioNoise(
          const rcSignals::SignalType volumeTypeVal,
          const NoiseType noiseVal,
          const std::array<Volume, 2> volumeVal) :
        noiseState(0),
        lastSample(0),
        freq(500u),
        volumeType(volumeTypeVal),
        noiseType(noiseVal) {
    volume = volumeVal;
    start();
}

void AudioNoise::start() {
    pos = 0.0f;
}

void AudioNoise::step(const rcProc::StepInfo& info) {

    rcSignals::RcSignal dynamicVolume;
    if (volumeType == SignalType::ST_NONE) {
        dynamicVolume = RCSIGNAL_MAX;
    } else {
        dynamicVolume = info.signals->get(volumeType, RCSIGNAL_NEUTRAL);
    }

    float fVolume = (dynamicVolume / 1000.0f);
    float posStep = static_cast<float>(freq) / static_cast<float>(rcAudio::SAMPLE_RATE);

    for (const auto interval : info.intervals) {
        copySamples(posStep, fVolume, interval);
    }
}

void AudioNoise::copySamples(
    const float posStep,
    const float dynamicVolume,
    const rcProc::SamplesInterval& interval) {

    auto first = interval.first;
    auto last = interval.last;

    while (first != last) {
        uint8_t sample = 0;

        switch (noiseType) {
        case NoiseType::WHITE:
            noiseState = hash(noiseState);
            sample = (noiseState & 0xFF);
            break;

        case NoiseType::PINK:
            noiseState = hash(noiseState);
            sample = (noiseState & 0xFF);
            if (abs(sample - lastSample) < 127) {
                sample = 255 - sample;
            }
            break;

        case NoiseType::COLOR:
            {
                bool needNoise = ((std::lround(pos * 2.0f) % 2) == 0);
                if (needNoise) {
                    noiseState = hash(noiseState);
                    sample = (noiseState & 0xFF);
                } else {
                    sample = lastSample;
                }
            }
            break;

        case NoiseType::SINE:

            sample = std::clamp(
                128.0f + 128.0f * sinf(pos * 2.0f * static_cast<float>(M_PI)),
                0.0f,
                255.0f);
            break;

        case NoiseType::SAWTOOTH:
            sample = std::min(256.0f * pos, 255.0f);
            break;

        case NoiseType::TRIANGLE:
            sample = std::clamp(
                (pos > 0.5f) ? (512.0f * (1.0f - pos)) : (512.0f * pos),
                0.0f,
                255.0f);
            break;

        case NoiseType::RECT:
            sample = (pos > 0.5f) ? 0u : 255u;
            break;
        }

        lastSample = sample;
        copySample(sample, first, dynamicVolume);

        pos += posStep;
        while (pos >= 1.0) {
            pos -= 1.0;
        }
        first++;
    }
}

} // namespace

