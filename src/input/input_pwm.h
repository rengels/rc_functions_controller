/**
 *  Implementation of the pwm input functionality.
 *
 *  @file
*/

#ifndef _INPUT_PWM_H
#define _INPUT_PWM_H

#include "input.h"
#include "input_ppm.h"
#include "signals.h"

#include <array>

namespace rcInput {


/** This class reads PWM input signals from up to 6 input pins.
 *
 *  This uses the InputPpm module multiple times to query
 *  all the input pins.
 *
 *  Note: The InputPwm and OutputPwm procs have a conflict of IO ports.
 */
class InputPwm : public Input {
    private:

        static constexpr uint8_t NUM_CHANNELS = 6U;

        /// Input pin numbers (pin 34 & 35 only usable as inputs!)
        static constexpr std::array<gpio_num_t, NUM_CHANNELS> PINS{
            GPIO_NUM_12,
            GPIO_NUM_13,
            GPIO_NUM_14,
            GPIO_NUM_27,
            GPIO_NUM_34,
            GPIO_NUM_35
        };
        std::array<InputPpm, NUM_CHANNELS> ppmModules;

        std::array<rcSignals::SignalType, NUM_CHANNELS> types;

    public:
        InputPwm();
        virtual ~InputPwm();

        virtual void start() override;
        virtual void stop() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const InputPwm&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, InputPwm&);
};

} // namespace

#endif // _INPUT_PWM_H
