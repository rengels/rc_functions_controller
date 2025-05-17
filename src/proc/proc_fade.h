/**
 *  This file contains definition for the Proc class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_FADE_H_
#define _RC_PROC_FADE_H_

#include "proc.h"

#include <cstdint>
#include <array>

class ProcTest_Fade_Test;

namespace rcProc {

/** Fade effect Proc
 *
 *  Slow down the change of a signal, as with the fading of an
 *  indacentend light bulb.
 *
 *  We try to avoid costly long int divisions by using the fixed fade amount.
 */
class ProcFade: public Proc{
    private:
        static constexpr uint8_t NUM_CHANNELS = 4U; ///< the number of channels for a single effect instance.

        /** Change of the signal every 10ms
         *
         *  - 1: full fade in 10s
         *  - 100: full fade in .1s
         */
        uint16_t fadeIn;

        /** Change of the signal every 10ms
         *
         *  - 1: full fade in 10s
         *  - 100: full fade in .1s
         */
        uint16_t fadeOut;

        std::array<rcSignals::RcSignal, NUM_CHANNELS> oldValues; ///< the value of the signal the last time around.

        std::array<rcSignals::SignalType, NUM_CHANNELS> types;

    public:
        ProcFade();
        ProcFade(uint16_t fadeInVal,
            uint16_t fadeOutVal,
            std::array<rcSignals::SignalType, NUM_CHANNELS> typesVal);

        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcFade&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcFade&);

        friend ProcTest_Fade_Test;
};

} // namespace

#endif // _RC_PROC_FADE_H_
