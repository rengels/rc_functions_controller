/* RC engine functions controller for Arduino ESP32.
 *
 * Simple audio proc
 *
 */

#ifndef _AUDIO_SIMPLE_H_
#define _AUDIO_SIMPLE_H_

#include "audio.h"
#include "signals.h"
#include <cstdint>

namespace rcAudio {

/** Simple sound module.
 *
 *  This is a simple sound with logic for triggering.
 */
class AudioSimple : public Audio {
    protected:
        SampleData sample;  ///< The sample that should be played.

        /** The signal type that will trigger the sound
         *
         *  The signal needs to go back to idle before the sound
         *  is played again.
         */
        rcSignals::SignalType triggerType;

        bool triggerOld;  ///< True if the sound is triggered (trigger signal is positive).
        bool active;      ///< True if the sound is played.
        uint32_t pos;     ///< Current playing position

        /** Copies the current samples (at *pos*) to the sample interval
         *
         *  @param triggerNew True if the trigger signal is currently active.
         *    Used in AudioLoop
         *  @param interval The samples interval that needs to be filled.
         *  @param volume The volume from the ST_MASTER_VOLUME signal.
         */
        virtual void copySamples(bool triggerNew,
            const rcProc::SamplesInterval& interval);

    public:
        AudioSimple();
        AudioSimple(const SampleData& sampleVal,
                    const rcSignals::SignalType typeVal,
                    const std::array<Volume, 2> volumeVal = {1.0f, 1.0f});
        virtual ~AudioSimple() {}

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const AudioSimple&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, AudioSimple&);
};

} // namespace

#endif // _AUDIO_SIMPLE_H_

