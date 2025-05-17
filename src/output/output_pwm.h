/**
 *  Implementation of the output via MCPWM (for PWM for servos)
 *
 *  @file
*/

#ifndef _OUTPUT_PWM_H
#define _OUTPUT_PWM_H

#include "output.h"
#include "signals.h"
#include <cstdint>
#include <array>

#include <driver/mcpwm_prelude.h>
#include <hal/gpio_types.h>

using namespace rcSignals;

namespace rcOutput {

class OutputEsc;

/** Concrete implementation of the Output interface
 *  for PWM.
 *
 *  This is a digital output capable of pwm duty cycle control.
 *  It uses:
 *
 *  - mcpwm group 0
 *  - GPIO_NUM_12
 *  - GPIO_NUM_13
 *  - GPIO_NUM_14
 *
 *  @see examples/peripherals/mcpwm/mcpwm_servo_control
 *
 *  Note: this class can be overridden to implement different
 *  PWM mechanisms.
 */
class OutputPwm : public Output {
    protected:
        /** Number of PWM channels
         *
         *  The pwm provides three comparators/generators per block.
         */
        static constexpr uint8_t PWM_NUM = 3;

        static constexpr uint32_t TIMEBASE_RESOLUTION_HZ = 1000000u;  ///< 1MHz, 1us per tick
        static constexpr uint32_t SERVO_TIMEBASE = 20000u;  ///< 20000us, 20ms (50Hz)

        /** The group ID used by this instance.
         *
         *  Every group provides three channels, and we have three groups
         *  available.
         */
        uint8_t groupId;

        mcpwm_timer_handle_t handleTimer;
        std::array<mcpwm_oper_handle_t, PWM_NUM> handleOper;
        std::array<mcpwm_cmpr_handle_t, PWM_NUM> handleCmpr;
        std::array<mcpwm_gen_handle_t, PWM_NUM> handleGen;

        /** Input types for the pwm channels.
         *
         *  If set to ST_NONE, then the pin will not be used and
         *  outputted.
         */
        std::array<SignalType, PWM_NUM> types;

        /** Output pins for the different PWM signals. */
        std::array<gpio_num_t, PWM_NUM> pins;

        /** Returns the number of ticks (microseconds for PWM) for a specific signal.
         *
         *  @param[in] index The pin/type/signal number.
         *  @param[in] signal The current value of the signal signal[types[index]]
         */
        uint32_t signalToUs(rcSignals::RcSignal signal) const;

    public:
        OutputPwm();

        virtual void start() override;
        virtual void stop() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const OutputPwm&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, OutputPwm&);

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const OutputEsc&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, OutputEsc&);
};

} // namespace

#endif  // _OUTPUT_PWM_H
