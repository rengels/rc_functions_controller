/* RC engine functions controller for Arduino ESP32.
 *
 * Steam audio proc
 *
 */

#include "audio_steam.h"
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

AudioSteam::AudioSteam(
    const uint16_t toneVal,
    const float offsetVal,
    const float cylinderResistanceVal,
    const float exaustResistanceVal,
    const std::array<Volume, 2> volumeVal) :
        pos(0.0f),
        cylinderPressure(0.0f),
        exaustPressure(0.0f),
        noiseState(0),
        lastSample(0),
        tone(toneVal),
        offset(offsetVal),
        cylinderResistance(cylinderResistanceVal),
        exaustResistance(exaustResistanceVal) {
    volume = volumeVal;
    start();
}

void AudioSteam::start() {
    cylinderPressure = 0.0f;
    exaustPressure = 0.0f;
    pos = 0.0f;
}

void AudioSteam::step(const rcProc::StepInfo& info) {

    const auto rpm = info.signals->get(SignalType::ST_RPM, rcSignals::RCSIGNAL_NEUTRAL);
    const auto throttle = info.signals->get(SignalType::ST_THROTTLE, RCSIGNAL_NEUTRAL);

    // mute volume at low throttles
    float fVolume = ((static_cast<float>(throttle) / 1000.0f) * 0.3f) + 0.7f;

    const float posStep = abs((rpm / 60.0f) / static_cast<float>(rcAudio::SAMPLE_RATE));

    for (const auto interval : info.intervals) {
        copySamples(posStep, fVolume, interval);
    }
}

void AudioSteam::copySamples(
    const float posStep,
    const float dynamicVolume,
    const rcProc::SamplesInterval& interval) {

    constexpr std::array<float, 4> valveTiming{0.2f, 0.44f, 0.68f, 0.92f};

    auto first = interval.first;
    auto last = interval.last;

    while (first != last) {

        // -- generate noise
        exaustPressure *= (1.0f - exaustResistance);
        exaustPressure += cylinderPressure * cylinderResistance;
        cylinderPressure *= (1.0f - cylinderResistance);

        float steamVolume = std::min(exaustPressure, 1.0f);

        // calculate noise
        uint16_t counter = (uint64_t)first;
        if (counter % tone == 0) {
            // get new random value
            noiseState = hash(noiseState);
        }

        // slowly change to new sample
        uint8_t sample = (noiseState & 0xFF);
        if (tone > 0) {
            int diff = static_cast<int>(sample) - static_cast<int>(lastSample);
            lastSample += (diff / tone);
        } else {
            lastSample = sample;
        }

        copySample(lastSample, first, steamVolume * dynamicVolume);

        // -- update timing
        float newPos = pos + posStep;
        for (const auto& timing : valveTiming) {
            if ((pos < timing) &&
                (newPos >= timing)) {

                if (timing == 0.2f) {
                    cylinderPressure = 1.0f;
                } else {
                    cylinderPressure = 0.5f;
                }
            }
        }

        pos += posStep;
        while (pos >= (1.0 + offset)) {
            pos -= 1.0;
        }
        first++;
    }
}

} // namespace

