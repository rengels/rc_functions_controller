/**
 *  This file contains definition for the ProcSequence class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_SEQUENCE_H_
#define _RC_PROC_SEQUENCE_H_

#include "proc.h"

#include <cstdint>
#include <array>

class ProcSequenceTest_Sequence_Test;

namespace rcProc {

/** This class implements a Proc that will output a sequence
 *  of on/off changes.
 *
 *  This effect can be used to implement the double-flash of
 *  US police car beacons and many other things.
 *
 *  Note: This proc can output on the input signal.
 *  Note: This proc can overwrite valid output signals.
 */
class ProcSequence: public Proc {
    private:
        rcSignals::TimeMs sequenceTimeMs; ///< The time since the start of the sequence

        /** The input signal that triggers the sequence.
         *
         *  The sequence will be repeated as long as the signal stays TRUE.
         */
        rcSignals::SignalType inputType;

        /** This is the output signal for the sequence.
         *
         *  Input and output signals can be the same.
         */
        rcSignals::SignalType outputType;

        /** This are the times between each signal change.
         *
         *  The time is in ms.
         *  The starting state is always "off"
         *  change can be used.
         **/
        std::array<uint16_t, 6> onOffTimes;

        /** The overall length of the sequence. */
        rcSignals::TimeMs sequenceDurationMs;

        /** Run as long as trigger is active instead of only once. */
        bool onlyOnce;

    public:
        ProcSequence();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend ProcSequenceTest_Sequence_Test;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcSequence&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcSequence&);
};


} // namespace

#endif // _RC_PROC_SEQUENCE_H_
