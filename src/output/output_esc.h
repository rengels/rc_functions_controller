/**
 *  Implementation of the output via MCPWM (for ESC for motors)
 *
 *  @file
*/

#ifndef _OUTPUT_ESC_H_
#define _OUTPUT_ESC_H_

#include "output_pwm.h"
#include "signals.h"

namespace rcOutput {

/** Enum for the output algorithm in OutputEsc
 *
 *  We need to declare it here, and not inside the class or else I could not
 *  forward declare it in simple_byte_stream.h
 */
enum class FreqType : uint8_t {
    KHZ10,
    KHZ5,
    KHZ1,
    HZ100,
    HZ10,
    HZ5
};

/** Concrete implementation of the Output interface
 *  for ESC.
 *
 *  This class just overrides a couple of functions of
 *  OutputPwm.
 *  Also we need additional pins for the motor control.
 *
 *  One pin will be hold high and low (for the direction) and
 *  the other one will have a 4kHz frequency.
 *
 *  (Benefits: we need just one pwm channel per motor. Also
 *   there is no frequency on the second port which has electrical
 *   benefits)
 */
class OutputEsc : public OutputPwm {

        /** Output pins for the second signal of the ESC.
         *
         *  The drivers shall be connected to pin[0] and pin2[0].
         */
        std::array<gpio_num_t, PWM_NUM> pins2;

        static constexpr uint8_t NUM_FREQ_SLOTS = 5u;

        /** The freq type for the signal range.
         *
         *  With 5 slots you can configure different frequencies for
         *  0 - 199, 200 - 399, ...
         *
         *  Specifically the low range can have a low frequency to allow
         *  the motor to start.
         */
        std::array<FreqType, NUM_FREQ_SLOTS> freqTypes;

        uint16_t deadZone;  ///< The minimal signal that produces an output

        /** Returns the us for a signal with a 1kHz signal */
        int16_t signalToPwm(const rcSignals::RcSignal signal) const;

        /** Controls the output by manually switching the timers on an off.
         *
         */
        void stepSlow(const rcProc::StepInfo& info, uint16_t stepIncrement);

        /** Configures the PWM for the output.
         *
         *  @param[in] period The timer period in ms.
         */
        void stepFast(const rcProc::StepInfo& info, uint32_t period);

    public:
        OutputEsc();

        virtual void start() override;
        virtual void stop() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const OutputEsc&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, OutputEsc&);
};

} // namespace

#endif  // _OUTPUT_ESC_H_
