/* RC functions controller for Arduino ESP32.
 *
 * Classes and structures dynamic sample output.
 *
 */

#ifndef _AUDIO_DYNAMIC_H_
#define _AUDIO_DYNAMIC_H_

#include "audio.h"
#include "signals.h"

#include <array>
#include <cstdint>

class AudioDynamicTest_getSampleIndices_Test;
class AudioDynamicTest_getVolumes_Test;

namespace rcAudio {

/** Audio module dynamic sound.
 *
 *  Plays sounds faster/slower and louder/quiter depending
 *  on input signals.
 *
 *  It also plays sounds in a loop, so it's like AudioLoop
 *  without the additional input signals.
 */
class AudioDynamic : public Audio {
    private:
        SampleData sample;

        /** The input signal determining the speed.
         *
         *  An input signal of 1000 meas 100% speed.
         */
        rcSignals::SignalType speedType;

        /** The input signal determining the volume.
         *
         *  1000 represents 100% volume (so volume can be more than 100%).
         *  Negative input signals will invert the waveform.
         */
        rcSignals::SignalType volumeType;

        float pos; ///< Current playing position (part of a revolution).

        /** Copy samples to the target buffer.
         *
         *  @param posStep increase of \ref pos per sample step
         *  @param interval The samples interval that needs to be filled.
         *  @param volume The volume from the ST_MASTER_VOLUME signal.
         */
        void copySamples(float posStep,
                         const rcProc::SamplesInterval& interval,
                         float volume);

    public:
        AudioDynamic();
        AudioDynamic(const SampleData& sampleVal,
                    const rcSignals::SignalType& speedTypeVal,
                    const rcSignals::SignalType& volumeTypeVal,
                    const std::array<Volume, 2> volumeVal = {1.0f, 1.0f});

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend AudioDynamicTest_getSampleIndices_Test;
        friend AudioDynamicTest_getVolumes_Test;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const AudioDynamic&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, AudioDynamic&);
};

}

#endif // _AUDIO_DYNAMIC_H_

