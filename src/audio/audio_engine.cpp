/* RC engine functions controller for Arduino ESP32.
 *
 * Engine audio proc
 *
 */

#include "audio_engine.h"
#include "signals.h"

#include <algorithm>  // for clamp
#include <cmath>  // for floor and abs and sqrtf
#include <cstdint>

using namespace rcSignals;

namespace rcAudio {

AudioEngine::AudioEngine() :
    throttleType(rcSignals::SignalType::ST_THROTTLE),
    samples{SampleData(), SampleData()},
    rpms{1.0},
    throttles{0},
    currentVolumes{0.0f} {

    volume = {1.0f, 1.0f};
    start();
}

AudioEngine::AudioEngine(const std::array<SampleData, NUM_SAMPLES>& samplesVal,
                         const std::array<rcSignals::RcSignal, NUM_SAMPLES>& throttlesVal,
                         const std::array<Volume, 2> volumeVal) :
    throttleType(rcSignals::SignalType::ST_THROTTLE),
    samples(samplesVal),
    rpms{1.0},
    throttles(throttlesVal),
    currentVolumes{0.0f} {

    volume = volumeVal;
    start();
}

void AudioEngine::start() {
    lastVolumeFactor = 0.0f;
    pos = 0.0f;

    // calculate RPMs
    //
    // the assumption is that the audio sample covers all cylinders.
    // That means that the RPM of the recorded sample
    // is:
    //
    // 60 / (num_samples / sample_rate)
    rpmMax = 0.0f;
    for (uint8_t i = 0; i < NUM_SAMPLES; i ++) {
        if (isValidSample(i)) {
            rpms[i] = 60.0f * rcAudio::SAMPLE_RATE / samples[i].size();
            rpmMax = std::max(rpmMax, rpms[i]);
        }
    }

    currentVolumes = {0.0f};
}


std::array<float, AudioEngine::NUM_SAMPLES> AudioEngine::getWeights(float rpm,
    rcSignals::RcSignal throttle) const {

    // in order to have a fair distance between
    // throttles (0 - 1000) and
    // rpms (0 - rpmMax)
    // we weight them
    float rpmFactor = 1000.0f / rpmMax;

    std::array<float, NUM_SAMPLES> weights;
    for (uint8_t i = 0u; i < NUM_SAMPLES; i ++) {
        if (isValidSample(i)) {
            weights[i] =
                std::abs(rpms[i] - rpm) * rpmFactor +
                std::abs(throttles[i] - throttle);
            weights[i] = sqrtf(weights[i]);  // better separation of samples, smoother transitions
        } else {
            weights[i] = 0.0f;
        }
    }

    return weights;
}


/** The volumes should be distributed between the different samples,
 *  according to the following rules:
 *
 *  - empty audios (currently defined as samples with less than 10 samples)
 *    should be ignored and get a volume of 0.0.
 *  - the volume should go mostly to the sample with the closest
 *    match of rpm and throttle.
 *  - in case of two samples being equal, the volumes should
 *    be distributed.
 */
std::array<float, AudioEngine::NUM_SAMPLES> AudioEngine::getVolumes(
    const float rpm,
    rcSignals::RcSignal throttle) const {

    // -- calculate weights
    auto weights = getWeights(rpm, throttle);

    float minWeight = 10000.0f;
    for (uint8_t i = 0u; i < NUM_SAMPLES; i ++) {
        if (isValidSample(i)) {
            minWeight = std::min(minWeight, weights[i]);
        }
    }

    // -- convert weights to volume
    // map weights to a reverse range (1.0f - 0.0f)
    float totalWeight = 0.0f;
    for (uint8_t i = 0u; i < NUM_SAMPLES; i ++) {
        if (isValidSample(i)) {
            weights[i] = 1.0f / (weights[i] - minWeight + 1.0f);
            totalWeight += weights[i];
        } else {
            weights[i] = 0.0f;
        }
    }

    // normalize weights to 1
    if (totalWeight > 0.0f) {
        for (auto& weight : weights) {
            weight /= totalWeight;
        }
    } else {
        for (auto& weight : weights) {
            weight = 1.0f;
        }
    }

    return weights;
}

void AudioEngine::step(const rcProc::StepInfo& info) {

    rcSignals::RcSignal throttle;
    if (throttleType == SignalType::ST_NONE) {
        throttle = RCSIGNAL_MAX;
    } else {
        throttle = info.signals->get(throttleType, RCSIGNAL_NEUTRAL);
    }

    auto rpm = info.signals->get(SignalType::ST_RPM, rcSignals::RCSIGNAL_NEUTRAL);

    // smooth blending
    // 0.01 -> 100 cycles -> 2 s
    if (rpm < 100) {
        lastVolumeFactor = std::clamp(lastVolumeFactor - 0.01f, 0.0f, 1.0f);
    // 0.1 -> 10 cycles -> 200 ms
    } else {
        lastVolumeFactor = std::clamp(lastVolumeFactor + 0.1f, 0.0f, 1.0f);
    }

    auto newVolumes = getVolumes(rpm, throttle);
    for (auto& volume : newVolumes) {
        volume *= lastVolumeFactor;
    }

    /* this is for debugging, to see the different volumes
    Signals& signals = *info.signals;
    signals[SignalType::ST_HYDRAULIC] = static_cast<int16_t>(newVolumes[0] * 1000.0);
    signals[SignalType::ST_BUCKET_RATTLE] = static_cast<int16_t>(newVolumes[1] * 1000.0);
    signals[SignalType::ST_TIRES] = static_cast<int16_t>(newVolumes[2] * 1000.0);
    signals[SignalType::ST_FUEL_EMPTY] = static_cast<int16_t>(newVolumes[3] * 1000.0);
    signals[SignalType::ST_WINCH] = static_cast<int16_t>(newVolumes[4] * 1000.0);
    */

    float posStep = abs((rpm / 60.0f) / static_cast<float>(rcAudio::SAMPLE_RATE));

    for (const auto interval : info.intervals) {
        copySamples(posStep, newVolumes, interval);
    }
}

void AudioEngine::copySamples(float posStep,
    const std::array<float, AudioEngine::NUM_SAMPLES>& newVolumes,
    const rcProc::SamplesInterval& interval) {

    auto first = interval.first;
    auto last = interval.last;

    // -- copy audio samples
    while (first != last) {

        for (uint8_t i = 0; i < NUM_SAMPLES; i ++) {
            if (currentVolumes[i] > 0.0f) {
                copySample(
                    samples[i][std::min(
                        static_cast<uint16_t>(samples[i].size() * pos),
                        static_cast<uint16_t>(samples[i].size() - 1))],
                    first,
                    currentVolumes[i]);
            }
        }

        pos += posStep;
        while (pos >= 1.0) {
            pos -= 1.0;
            currentVolumes = newVolumes;
        }
        first++;
    }
}

} // namespace

