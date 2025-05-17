/**
 *  Implementation of the ADC input functionality.
 *
 *  @file
*/

#include "input_adc.h"
#include "signals.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

const static char *TAG = "inADC";

#define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_12

using namespace rcSignals;
using namespace rcProc;

namespace rcInput {

InputAdc::InputAdc():
            adcHandle(nullptr),
            calibrationHandle(nullptr),
            floatingAverage(-1.0f),
            pin(GPIO_NUM_39),
            type(rcSignals::SignalType::ST_VCC)
            {
}

InputAdc::~InputAdc() {
    stop();
}

adc_unit_t InputAdc::unitForPin(gpio_num_t pin) {
    switch (pin) {
    case GPIO_NUM_32:
    case GPIO_NUM_33:
    case GPIO_NUM_34:
    case GPIO_NUM_35:
    case GPIO_NUM_36:
    case GPIO_NUM_37:
    case GPIO_NUM_38:
    case GPIO_NUM_39:
        return ADC_UNIT_1;
    default:
        return ADC_UNIT_2;
    }
}

adc_channel_t InputAdc::channelForPin(gpio_num_t pin) {
    switch (pin) {
    case GPIO_NUM_32:
        return ADC_CHANNEL_4;
    case GPIO_NUM_33:
        return ADC_CHANNEL_5;
    case GPIO_NUM_34:
        return ADC_CHANNEL_6;
    case GPIO_NUM_35:
        return ADC_CHANNEL_7;
    case GPIO_NUM_36:
        return ADC_CHANNEL_0;
    case GPIO_NUM_37:
        return ADC_CHANNEL_1;
    case GPIO_NUM_38:
        return ADC_CHANNEL_2;
    case GPIO_NUM_39:
        return ADC_CHANNEL_3;
    default:
        return ADC_CHANNEL_0;
    }
}

adc_cali_handle_t InputAdc::setupCalibration(const adc_unit_t unit,
    const adc_channel_t channel,
    const adc_atten_t atten) {

    adc_cali_handle_t handle = nullptr;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .default_vref = 0,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    }
#endif

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return handle;
}

/** Reserve resources for ADC input, activate the calibration
 *
 *  Note: ADC_UNIT_2 is considered invalid, since it's already used by WIFI
 */
void InputAdc::start() {
    if (adcHandle == nullptr &&
      unitForPin(pin) != ADC_UNIT_2) {
        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = unitForPin(pin),
            .clk_src = static_cast<adc_oneshot_clk_src_t>(0),
            .ulp_mode = static_cast<adc_ulp_mode_t>(0),
        };
        ESP_ERROR_CHECK(
            adc_oneshot_new_unit(&init_config1, &adcHandle));

        adc_oneshot_chan_cfg_t config = {
            .atten = EXAMPLE_ADC_ATTEN,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ESP_ERROR_CHECK(
            adc_oneshot_config_channel(adcHandle, channelForPin(pin), &config));

        // calibration
        calibrationHandle = setupCalibration(
            unitForPin(pin),
            channelForPin(pin), EXAMPLE_ADC_ATTEN);
    }
}


/** Undo everything from the start() function.
 *
 *  Note: we can leave the pins at input.
 */
void InputAdc::stop() {
    if (adcHandle != nullptr) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
        ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(calibrationHandle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
        ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(calibrationHandle));
#endif

        ESP_ERROR_CHECK(
            adc_oneshot_del_unit(adcHandle));

        adcHandle = nullptr;
    }
}

/** Receive the raw signals from ADC input.
 *
 *  Check for each channel, if the recording has happened.
 *  In that case we will convert the times to signal values and
 *  restart the channel.
 *
 *  Call this function around once every 20ms.
 */
void InputAdc::step(const StepInfo& info) {
    if (adcHandle != nullptr) {
        static int count = 0;
        int raw;
        int voltage;

        ESP_ERROR_CHECK(
            adc_oneshot_read(adcHandle,
                channelForPin(pin),
                &raw));

        if (calibrationHandle) {
            ESP_ERROR_CHECK(
                adc_cali_raw_to_voltage(
                    calibrationHandle,
                    raw,
                    &voltage));
        } else {
            voltage = raw;
        }

        // -- calculate a floating average
        if (floatingAverage < 0.0f) { // first meassurement
            floatingAverage = voltage;
        } else {
            floatingAverage = (floatingAverage * 5 + voltage) / 6.0f;
        }

        if (type != rcSignals::SignalType::ST_NONE) {
            info.signals->safeSet(type, floatingAverage);
        }

        // TODO: debug
        count++;
        if (count > 1000) {
            ESP_LOGI(TAG, "ADC raw: %d, volt %d, average: %f", raw, voltage, floatingAverage);
            count = 0;
        }
    }
}

} // namespace

