/* RC functions controller for Arduino ESP32.
 *
 * Class for playing engine sounds.
 *
 */

#ifndef _AUDIO_ENGINE_H_
#define _AUDIO_ENGINE_H_

#include "audio.h"
#include "signals.h"

#include <array>
#include <cstdint>

class AudioEngineTest_getVolumes_Test;

namespace rcAudio {

/** Audio module for engine
 *
 *  Plays sounds depending on the RPM.
 *
 */
class AudioEngine : public Audio {
    private:
        static constexpr uint8_t NUM_SAMPLES = 5;

        /** The input signal determining the throttle.
         *
         *  Usually this is ST_THROTTLE, but for the Jack-brake
         *  it could be ST_BRAKE.
         *
         *  The throttle determines the samples being used
         *  for the playback.
         */
        rcSignals::SignalType throttleType;

        std::array<SampleData, NUM_SAMPLES> samples;

        /** The RPMs for the samples.
         *
         *  This will be computed in the start() function.
         */
        std::array<float, NUM_SAMPLES> rpms;

        /** The max RPM of all valid samples.
         *
         *  This will be computed in the start() function.
         */
        float rpmMax;

        /** The throttles for the samples.
         */
        std::array<rcSignals::RcSignal, NUM_SAMPLES> throttles;

        /** The volumes currently used for all the samples.
         *
         *  To prevent clicking we exchange the volumes only
         *  at the start of the samples.
         */
        std::array<float, NUM_SAMPLES> currentVolumes;

        /** Used for smooth blending volume. */
        float lastVolumeFactor;

        float pos; ///< Current playing position (part of a engine revolution).

        /** Returns of the sample with the index is considered valid.
         *
         *  Samples that are too short (e.g. the "silence" sample) are
         *  not considered for any calculation.
         */
        bool isValidSample(uint8_t index) const {
            return samples[index].size() > 9u;
        };

        /** Determines the relative distance of the samples form the given
         *  rpm and throttle.
         *
         *  Weight is the taxi distance between sample RPM/throttle and actual
         *  RPM/throttle
         *
         *  @param[in] rpm The RPM for which the volumes should be calculated.
         *  @param[in] throttle The throttle for which the volumes should be calculated.
         */
        std::array<float, NUM_SAMPLES> getWeights(float rpm, rcSignals::RcSignal throttle) const;

        /** Determines the relative volume for the sample indices
         *
         *  @param[in] rpm The RPM for which the volumes should be calculated.
         *  @param[in] throttle The throttle for which the volumes should be calculated.
         */
        std::array<float, NUM_SAMPLES> getVolumes(float rpm, rcSignals::RcSignal throttle) const;

        /** Copy samples to the target buffer.
         *
         *  @param[in] posStep increase of \ref pos per sample step
         *  @param[in] newVolumes The volumes that should replace \ref currentVolumes
         *    once the engine revolution starts again.
         *  @param[in] interval The samples interval that needs to be filled.
         */
        void copySamples(float posStep,
                         const std::array<float, NUM_SAMPLES>& newVolumes,
                         const rcProc::SamplesInterval& interval);

    public:
        AudioEngine();
        AudioEngine(const std::array<SampleData, NUM_SAMPLES>& samplesVal,
                    const std::array<rcSignals::RcSignal, NUM_SAMPLES>& throttlesVal = {rcSignals::RCSIGNAL_NEUTRAL},
                    const std::array<Volume, 2> volumeVal = {1.0f, 1.0f});

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend AudioEngineTest_getVolumes_Test;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const AudioEngine&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, AudioEngine&);
};

}

#endif // _AUDIO_ENGINE_H_

