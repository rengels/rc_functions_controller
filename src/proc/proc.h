/**
 *  This file contains definition for the Proc class
 *  with the rc_functions_controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_H_
#define _RC_PROC_H_

#include "signals.h"
#include <array>

class SimpleInStream;
class SimpleOutStream;

namespace rcProc {

/** Identifier for the different proc types when serializing
 *  and deserializing.
 */
enum class ProcType: uint8_t;

/** Represents one sample (with two channels)
 *
 *  The channel values are supposed to be -127 to +127
 *  Values will be truncated when transfering them to the DMA buffer.
 *
 */
struct AudioSample {
    int16_t channel1;  ///< sample for channel 1
    int16_t channel2;  ///< sample for channel 2
};

/** Represents an interval of audio samples.
 *
 *  Used in the step function.
 */
struct SamplesInterval {
    /** Pointers to the samples in the audio buffer.
     *
     *  An audio proc should add it's samples from first to last.
     */
    AudioSample* first;

    /** Pointers to the last sample in the audio buffer (excluding).
     */
    AudioSample* last;
};

/** Information structure for the step function of the proc.
 *
 *  This information is filled out by the main loop before
 *  calling the proc functions.
 */
struct StepInfo {
    /** The time between the last call of the main function and the current call.
     *
     *  Time in milli seconds.
     */
    rcSignals::TimeMs deltaMs;


    /** The input/output signals.
     *
     *  It is expected that input and *normal* proc functions modify the
     *  signals.
     *  Output and audio procs usually don't modify the signals, althoug
     *  they could.
     */
    rcSignals::Signals* signals;

    /** Empty audio buffer intervals.
     *
     *  Audio procs should fill the intervals with their output.
     *  OutputAudio will play back the audio contained in the intervals
     *  eventually.
     */
    std::array<SamplesInterval, 2> intervals;
};

/** This class *processes* signals in one way or another.
 *
 *  This is the abstract baseclass of all the processors.
 *  A processor does some operation on signals, e.g. creating
 *  signals or modifying them in some way.
 */
class Proc {
    public:
        virtual ~Proc() {
            stop();
        }

        /** Initializes this module.
         *
         *  Called in constructor and deserialize() function.
         *  This might initialize variable or reserve CPU resources.
         */
        virtual void start() {}

        /** De-initialize this module.
         *
         *  This might free CPU resources and/or do any necessary cleanup.
         *  Also called in the deserialize() function before starting
         *  again.
         */
        virtual void stop() {}

        /** Executes a step for the proc
         *
         *  This function will:
         *  - optionally read input signals
         *  - does whathever this proc is supposed to do in a step
         *  - optionally write output signals
         *  - optionally write audio samples
         *
         *  This function needs to be called sufficiently often
         *  for short effects to work correctly, e.g. 15-30 ms.
         *
         *  @param[in,out] info The input/output signals.
         */
        virtual void step(const StepInfo& info) = 0;
};

} // namespace

#endif // _RC_PROC_H_
