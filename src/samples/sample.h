/** File contains the definitions for the audio samples.
 *
 * @file
 */

#ifndef _SAMPLE_SAMPLE_H_
#define _SAMPLE_SAMPLE_H_

#include <cstdint>
#include <array>
#include <span>

/** This namespace contains the sample related stuff used by the audio procs */
namespace rcSamples {

/** This type is used to uniqly identify audio samples.
 */
typedef std::array<char, 3> AudioId;

/** Struct for audio sample definition.
 *
 *  This might be raw audio data but usually it is
 *  a 22050 Hz unsigned 8 bit WAV file.
 */
struct SampleFile {
public:
    AudioId id;
    std::span<const uint8_t> content;  ///< sample file content/data
};

/** List of compiled in samples.
 */
const std::span<const rcSamples::SampleFile>& getStaticSamples();

} // namespace

inline bool operator==(const rcSamples::AudioId& id1, const rcSamples::AudioId& id2) {
    return ((id1[0] == id2[0]) &&
            (id1[1] == id2[1]) &&
            (id1[2] == id2[2]));
}


#endif // _SAMPLE_SAMPLE_H_

