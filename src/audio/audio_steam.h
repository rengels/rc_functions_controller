/* RC engine functions controller for Arduino ESP32.
 *
 * An audio module that produces a steam engine noise
 */

#ifndef _AUDIO_STEAM_H_
#define _AUDIO_STEAM_H_

#include "audio.h"
#include "signals.h"
#include <cstdint>

namespace rcAudio {

/** Steam sound module.
 *
 *  Synthesizes a steam engine sound using a noise generator.
 */
class AudioSteam : public Audio {
    public:

        float pos; ///< Current playing position (part of a engine revolution).

        /** A number for the steam contained in the cylinder.
         *
         *  This will be slowly released to the exaust via cylinderResistance.
         */
        float cylinderPressure;

        /** A number for the steam contained in the exaust.
         *
         *  This will be slowly released to the environment via exaustResistance.
         */
        float exaustPressure;

        /** A variable to store the random state. */
        uint32_t noiseState;

        /** The last sample that we outputted. */
        uint8_t lastSample;


        /** The frequency of the hissing. Same as with AudioNoise::COLOR */
        uint8_t tone;

        /** The offset in the cycle for this cylinder. 0.25 for 90 deg. */
        float offset;

        /** A value how fast the steam is released to the exaust. */
        float cylinderResistance;

        /** A value how fast the steam is released to the environment. */
        float exaustResistance;

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
        AudioSteam(
                   const uint16_t toneVal = 2u,
                   const float offsetVal = 0.0f,
                   const float cylinderResistanceVal = 0.01f,
                   const float exaustResistanceVal = 0.002f,
                   const std::array<Volume, 2> volumeVal = {1.0f, 1.0f});

        virtual ~AudioSteam() {}

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const AudioSteam&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, AudioSteam&);
};

} // namespace

#endif // _AUDIO_STEAM_H_

