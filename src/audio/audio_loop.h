/* RC functions controller for Arduino ESP32.
 *
 * Classes and structures for sound sample management.
 *
 */

#ifndef _AUDIO_LOOP_H_
#define _AUDIO_LOOP_H_

#include "audio_simple.h"
#include "signals.h"
#include <cstdint>

namespace rcAudio {

/** Looped sound module
 *
 *  This is in principle a AudioSimple, but it keeps playing
 *  while the trigger signal is there.
 */
class AudioLoop : public AudioSimple {
    protected:
        uint32_t loopBegin; ///< Start sample of the loop
        uint32_t loopEnd;   ///< End sample of the loop (exclusive)

        virtual void copySamples(bool triggerNew, const rcProc::SamplesInterval& interval) override;

    public:
        AudioLoop() :
            AudioSimple(),
            loopBegin(0),
            loopEnd(0) {
        }

        AudioLoop(const SampleData sampleVal,
                  uint32_t loopBeginVal,
                  uint32_t loopEndVal,
                  const rcSignals::SignalType typeVal,
                  const std::array<Volume, 2> volumeVal = {1.0f, 1.0f}) :
            AudioSimple(sampleVal, typeVal, volumeVal),
            loopBegin(loopBeginVal),
            loopEnd(loopEndVal) {
            }

        virtual ~AudioLoop() {}

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const AudioLoop&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, AudioLoop&);
};

}

#endif // _AUDIO_LOOP_H_

