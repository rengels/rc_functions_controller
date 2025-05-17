/* RC engine functions controller for Arduino ESP32.
 *
 * A proc module that triggers an output signal during a revolution.
 */

#ifndef _PROC_PERIODIC_H_
#define _PROC_PERIODIC_H_

#include "proc.h"
#include "signals.h"
#include <cstdint>

namespace rcProc {

/** Periodic module
 *
 *  Triggers a periodic signal with the frequency controlled by an input
 *  signal.
 */
class ProcPeriodic : public Proc {
    public:

        float pos;  ///< Current position in the cycle (0.0 - 1.0)

        rcSignals::SignalType freqType;  ///< The signal type determining the frequency
        rcSignals::SignalType outType;  ///< The signal type for the output

        /** A value that the input frequency is multiplied with. */
        float freqMultiplier;

        /** The offset in the cycle for this frequency. 0.25 for 90 deg. */
        float offset;

    public:
        ProcPeriodic(
                   const rcSignals::SignalType freqTypeVal = rcSignals::SignalType::ST_RPM,
                   const rcSignals::SignalType outTypeVal = rcSignals::SignalType::ST_AUX1,
                   const float freqMultiplierVal = (1.0f / 60.0f),
                   const float offsetVal = 0.0f);

        virtual ~ProcPeriodic() {}

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcPeriodic&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcPeriodic&);
};

} // namespace

#endif // _PROC_PERIODIC_H_

