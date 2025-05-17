/* RC engine functions controller for Arduino ESP32.
 *
 * An audio module that produces different types of
 * noises.
 *
 */

#ifndef _AUDIO_NOISE_H_
#define _AUDIO_NOISE_H_

#include "audio.h"
#include "signals.h"
#include <cstdint>

namespace rcAudio {

/** Noise sound module.
 *
 *  This is a noise sound generator, e.g. for hissing
 *  steam, fan or hydraulic flow sound.
 */
class AudioNoise : public Audio {
    public:
        /** Type of noise. */
        enum class NoiseType : uint8_t {
            WHITE = 0,  ///< White random noise
            PINK,
            COLOR,  ///< colored noise influenced by freq
            SINE,
            SAWTOOTH,
            TRIANGLE,
            RECT
        };

    protected:
        /** A variable to store the random state. */
        uint32_t noiseState;

        /** The last sample that we outputted. */
        uint8_t lastSample;

        /** The frequency of some of the noise types
         */
        uint16_t freq;

        /** The signal type that will trigger the sound.
         *
         *  The output volume is scaled with the value
         *  of the signal, -RCSIGNAL_MAX and RCSIGNAL_MAX being
         *  100% volume.
         */
        rcSignals::SignalType volumeType;

        /** The type of noise produced */
        NoiseType noiseType;

        float pos; ///< Current playing position (part of a engine revolution).

        /** Copies the current samples (at *pos*) to the sample interval
         *
         *  Actually this does not really copy samples, but generate
         *  them to fill the interval.
         *
         *  @param[in] posStep The fraction of the engine revolution per sample..
         *  @param[in] dynamicVolume A value from 0.0 to 1.0 representing the
         *    volume.
         *  @param[in] interval The samples interval that needs to be filled.
         */
        virtual void copySamples(
            const float posStep,
            const float dynamicVolume,
            const rcProc::SamplesInterval& interval);

    public:
        AudioNoise();
        AudioNoise(
                   const rcSignals::SignalType volumeTypeVal,
                   const NoiseType noiseVal,
                   const std::array<Volume, 2> volumeVal = {1.0f, 1.0f});
        virtual ~AudioNoise() {}

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const AudioNoise&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, AudioNoise&);
};

} // namespace

#endif // _AUDIO_NOISE_H_

