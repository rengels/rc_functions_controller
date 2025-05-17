/**
 *  Implementation of the output via LEDC.
 *
 *  @file
*/

#ifndef _OUTPUT_LED_H_
#define _OUTPUT_LED_H_

#include "output.h"
#include "signals.h"
#include <cstdint>
#include <array>

#include <driver/ledc.h>

using namespace rcSignals;

namespace rcOutput {

/** Concrete implementation of the Output interface
 *  via LEDC (meaning LED control module).
 *
 *  This is a digital output capable of pwm duty cycle control.
 *  It uses different output pins and one timer module.
 *
 *  The LEDC module supports two times 8 channels (low speed and
 *  high speed)
 */
class OutputLed : public Output {
    private:
        struct StaticConfig {
            gpio_num_t pin;
            ledc_channel_t channel;
            ledc_mode_t mode;
        };

        static constexpr uint8_t LEDC_NUM = 13; ///< number of ledc channels;

        static constexpr ledc_timer_t LEDC_TIMER = LEDC_TIMER_0;
        static constexpr ledc_mode_t LEDC_MODE = LEDC_LOW_SPEED_MODE;

        static constexpr ledc_timer_bit_t LEDC_DUTY_RES = LEDC_TIMER_13_BIT;
        static constexpr uint32_t LEDC_MAX_DUTY = 8191; ///< maximum duty
        static constexpr uint32_t LEDC_FREQUENCY = 5000U; // Frequency in Hertz. Set frequency at 5 kHz

        static constexpr StaticConfig CONFIG[] = {
            {GPIO_NUM_2,  LEDC_CHANNEL_0, LEDC_LOW_SPEED_MODE},
            {GPIO_NUM_3,  LEDC_CHANNEL_1, LEDC_LOW_SPEED_MODE},
            {GPIO_NUM_4,  LEDC_CHANNEL_2, LEDC_LOW_SPEED_MODE},
            {GPIO_NUM_5,  LEDC_CHANNEL_3, LEDC_LOW_SPEED_MODE},
            {GPIO_NUM_15, LEDC_CHANNEL_4, LEDC_LOW_SPEED_MODE},
            {GPIO_NUM_16, LEDC_CHANNEL_5, LEDC_LOW_SPEED_MODE},
            {GPIO_NUM_17, LEDC_CHANNEL_6, LEDC_LOW_SPEED_MODE},
            {GPIO_NUM_18, LEDC_CHANNEL_7, LEDC_LOW_SPEED_MODE},

            {GPIO_NUM_19, LEDC_CHANNEL_1, LEDC_HIGH_SPEED_MODE},
            {GPIO_NUM_21, LEDC_CHANNEL_2, LEDC_HIGH_SPEED_MODE},
            {GPIO_NUM_22, LEDC_CHANNEL_3, LEDC_HIGH_SPEED_MODE},
            {GPIO_NUM_23, LEDC_CHANNEL_4, LEDC_HIGH_SPEED_MODE},
            {GPIO_NUM_32, LEDC_CHANNEL_5, LEDC_HIGH_SPEED_MODE},
        };

        std::array<SignalType, LEDC_NUM> types;

    public:
        OutputLed():
            types {
                SignalType::ST_INDICATOR_LEFT,  // pin 2
                SignalType::ST_LOWBEAM,         // pin 3
                SignalType::ST_INDICATOR_RIGHT, // pin 4
                SignalType::ST_ROOF,            // pin 5
                // some pins have a double use (PWM)
                SignalType::ST_TAIL,            // pin 15
                SignalType::ST_FOG,             // pin 16
                SignalType::ST_REVERSING,       // pin 17
                SignalType::ST_SIDE,            // pin 18
                SignalType::ST_BEACON1,         // pin 19
                // unused 20
                SignalType::ST_BEACON2,         // pin 21
                SignalType::ST_CABIN,           // pin 22
                SignalType::ST_SHAKER,          // pin 23

                SignalType::ST_BRAKE            // pin 32
            } {
        }

        virtual void start() override;
        virtual void stop() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const OutputLed&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, OutputLed&);
};

} // namespace

#endif  // _OUTPUT_LED_H_
