/**
 *  This file contains definition for the Proc class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_RANDOM_H_
#define _RC_PROC_RANDOM_H_

#include "proc.h"

#include <cstdint>

namespace rcProc {

/** Random effect Proc
 *
 *  This proc will generate a random value every couple of cycles.
 *
 *  You can combine it with ProcFade to get a smooth random value.
 */
class ProcRandom: public Proc{
    private:
        /** Time between new random values in milliseconds.
         */
        uint16_t intervalMs;

        /** Internal value for the remaining time in milliseconds.
         */
        uint16_t remainingTimeMs;

        /** The last generated random value. */
        rcSignals::RcSignal lastValue;

        rcSignals::SignalType type;

    public:
        ProcRandom();
        ProcRandom(uint16_t intervalMsVal, rcSignals::SignalType typeVal);

        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcRandom&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcRandom&);

};

} // namespace

#endif // _RC_PROC_RANDOM_H_
