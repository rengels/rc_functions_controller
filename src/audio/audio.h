/* RC engine functions controller for Arduino ESP32.
 *
 * Classes and structures for audio sample management.
 *
 */

#ifndef _RC_AUDIO_H_
#define _RC_AUDIO_H_

#include "proc.h"
#include <cstdint>
#include <array>
#include <span>

/** Containing audio related functions, mostly the audio processing procs like ProcAudioSimple */
namespace rcAudio {

static constexpr uint32_t SAMPLE_RATE = 22050; // fixed sample rate for audio

typedef std::span<const uint8_t> SampleData;

/** Volume type for audio volume.
 *
 *  Usually 0.0 to 1.0, although >1.0 is also possible.
 *  Values above 2.55 cannot be serialized.
 *
 *  For serialization this is put into a uint8_t.
 *
 *  It needs to be a class or the operator overloading of SimpleStream
 *  would clash with "float".
 */
class Volume {
public:
    float value;

    Volume(float val = 0.0f) : value(val) {
    }
};

/** Base class for audios
 *
 *  Abstract base class for audio.
 *
 */
class Audio: public rcProc::Proc {
protected:
    /** Relative volume of the Audio (proc).
     *
     *  One per audio channel
     */
    std::array<Volume, 2> volume;

    /** Helper function to copy data from an SampleData to the audio ringbuffer.
     *
     *  This function considers the _flags_ and _volume_.
     *
     *  @param data The sample data straight out of the unsigned
     *     8bit WAV file.
     */
    void copySample(const uint8_t data, rcProc::AudioSample* samplePos) {
        float sampleData = data - 128;
        samplePos->channel1 += sampleData * volume[0].value;
        samplePos->channel2 += sampleData * volume[1].value;
    }

    /** Same, but with an additional volume.
     *
     *  @param dynamicVolume Volume multiplicator for the samples (e.g. 1.0f).
     */
    void copySample(const uint8_t data, rcProc::AudioSample* samplePos, const float& dynamicVolume) {
        float sampleData = (data - 128) * dynamicVolume;
        samplePos->channel1 += sampleData * volume[0].value;
        samplePos->channel2 += sampleData * volume[1].value;
    }

public:
    virtual ~Audio() {}
};


}

#endif // _RC_AUDIO_H_

