/* RC functions controller for Arduino ESP32.
 *
 * Play logic for the AudioLoop
 */

#include "audio_loop.h"

namespace rcAudio {

void AudioLoop::copySamples(bool triggerNew,
    const rcProc::SamplesInterval& interval) {

    auto first = interval.first;
    auto last = interval.last;

    // -- copy audio samples
    while (active && first != last && pos < sample.size()) {
        copySample(sample[pos], first);

        pos++;
        first++;

        // loop back
        if (triggerNew &&
            ((pos >= loopEnd) || (pos >= sample.size()))) {
            pos = loopBegin;
        }
    }
}

} // namespace

