/**
 *  This file contains definition for the ProcSwitch class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_SWITCH_H_
#define _RC_PROC_SWITCH_H_

#include "proc.h"

#include <cstdint>
#include <array>

class ProcSwitchTest_Debouncing_Test;
class ProcSwitchTest_Pos_Test;
class ProcSwitchTest_Output_Test;

namespace rcProc {

/** Implements a five-way switch with different modes.
 *
 *  The different outputs are switched or toggled depending on the
 *  input signal.
 *  The whole range of the signal is divided in five positions.
 *
 *  - momentary: The output signal is set as long as the input is in the position
 *  - short: The output signal is toggled if the input is in position for a short time (.5s to 1.5s)
 *  - long: The output signal is toggled if the input is in position for a long time (>1.5s)
 *
 *
 */

class ProcSwitch: public Proc {
    public:
        static constexpr uint8_t NUM_CHANNELS = 5U; ///< the number of channels in this case the number of different switch positions. Should be odd.

        /** A small helper struct to combine a stick / signal position and it's active time. */
        struct PosState {
            /** Position in this state. NUM_CHANNELS is an invalid position. */
            int8_t pos;

            /** The time that this position remained active */
            rcSignals::TimeMs time;

            bool isValid() const {
                return pos < NUM_CHANNELS;
            }

            void invalidate() {
                pos = NUM_CHANNELS;
            }

            bool operator==(const ProcSwitch::PosState& rhs) {
                return pos == rhs.pos;
            }

            bool operator!=(const ProcSwitch::PosState& rhs) {
                return pos != rhs.pos;
            }
        };

    private:
        static const rcSignals::TimeMs TIME_MS_DEBOUNCE = 50; ///< Time in milli seconds until the input is considered debounced (momentary)
        static const rcSignals::TimeMs TIME_MS_TOGGLE = 500; ///< Time in milli seconds until the input is considered toggled.
        static const rcSignals::TimeMs TIME_MS_TOGGLE_LONG = 1500; ///< Time in milli seconds until the input is considered toggled long (and not short).

        /** Last position (stick position) */
        PosState posLast;

        /** Last debounced stick position */
        PosState posDebouncedLast;

        /** The input signal for the switch */
        rcSignals::SignalType inType;

        /** List of channels for to set once the condition is detected.
         */
        std::array<rcSignals::SignalType, NUM_CHANNELS> outTypesMomentary;

        /** List of channels for to set once the condition is detected.
         */
        std::array<rcSignals::SignalType, NUM_CHANNELS> outTypesShort;

        /** List of channels for to set once the condition is detected.
         */
        std::array<rcSignals::SignalType, NUM_CHANNELS> outTypesLong;

        // cache for the toggle values
        std::array<bool, NUM_CHANNELS> signalMomentary;
        std::array<bool, NUM_CHANNELS> signalShort;
        std::array<bool, NUM_CHANNELS> signalLong;

    public:

        ProcSwitch();

        virtual void start() override;
        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcSwitch&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcSwitch&);

        friend ProcSwitchTest_Debouncing_Test;
        friend ProcSwitchTest_Pos_Test;
        friend ProcSwitchTest_Output_Test;
};



} // namespace

#endif // _RC_PROC_SWITCH_H_
