/**
 *  This file contains definition for the ProcCranking class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_CRANKING_H_
#define _RC_PROC_CRANKING_H_

#include "proc.h"

#include <cstdint>
#include <array>

namespace rcProc {

/** This class provides a cranking effect which will dim the brightness
 *  of lights while the cranking (ST_CRANKING) signal is on.
 */
class ProcCranking: public Proc {
    private:
        /** The amount that the signals are dimmed during cranking. */
        static constexpr rcSignals::RcSignal CRANKING_DIM = 400U;

        /** These are the signal types that are dimmed during cranking */
        std::array<rcSignals::SignalType, 4> types;

    public:
        ProcCranking():
            types{rcSignals::SignalType::ST_HIGHBEAM,
                rcSignals::SignalType::ST_LOWBEAM,
                rcSignals::SignalType::ST_ROOF,
                rcSignals::SignalType::ST_TAIL}
        {}

        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcCranking&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcCranking&);
};

} // namespace

#endif // _RC_PROC_CRANKING_H_
