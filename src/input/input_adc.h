/**
 *  Implementation of the ADC input functionality.
 *
 *  @file
*/

#include "input.h"
#include "signals.h"

#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <hal/gpio_types.h>


namespace rcInput {

/** This class reads ADC input signals one pin.
 *
 *  The signal value is not clamped to (-1000, 1000).
 *  Instead it's the raw value (or calibrated voltage if available).
 *
 *  This uses the ADC one shot module and ADC calibration.
 *  Only pins 32 to 39 are valid, since we only have ADC1
 *  (Note: ADC2 is used by WIFI)
 *
 *  @see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/adc_oneshot.html
 *  @see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html
 *
 */
class InputAdc : public Input {
    private:
        adc_oneshot_unit_handle_t adcHandle;
        adc_cali_handle_t calibrationHandle;

        /** Returns a adc unit for the given pin.
         *
         *  A result of ADC_UNIT_2 is also used for invalid pins.
         */
        static adc_unit_t unitForPin(gpio_num_t pin);

        /** Returns a adc channel for the given pin */
        static adc_channel_t channelForPin(gpio_num_t pin);

        /** Sets up the calibration for the given channel.
         *
         *  @returns the calibration handle or nullptr if calibration is not available.
         */
        static adc_cali_handle_t setupCalibration(const adc_unit_t unit,
                                                  const adc_channel_t channel,
                                                  const adc_atten_t atten);

        float floatingAverage;

        /** Again, only pins 32 to 39 are valid. */
        gpio_num_t pin;
        rcSignals::SignalType type;

    public:
        InputAdc();
        virtual ~InputAdc();

        virtual void start() override;
        virtual void stop() override;

        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const InputAdc&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, InputAdc&);
};

} // namespace

