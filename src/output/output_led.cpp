/**
 *  Implementation of the output via LEDC.
 *
 *  @file
*/

#include "output_led.h"
#include "signals.h"
#include <cstdint>

#include <driver/ledc.h>
#include <esp_err.h>

#include <esp_log.h>

static const char* TAG = "OutLed";

using namespace rcSignals;

namespace rcOutput {

/** Initializes this LED output module.
 *
 *  LED output uses LEDC_TIMER0
 *  and LEDC_CHANNEL 1-13
 */
void OutputLed::start() {

    ESP_LOGI(TAG, "Start configuring LED");

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK,
        .deconfigure      = false,
    };
    ESP_ERROR_CHECK(
        ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    for (uint8_t i = 0; i < LEDC_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }

        ESP_LOGI(TAG, "Configure channel %d", static_cast<int>(i));
        const StaticConfig &config = CONFIG[i];
        ledc_channel_config_t ledc_channel = {
            .gpio_num       = config.pin,
            .speed_mode     = config.mode,
            .channel        = config.channel,
            .intr_type      = LEDC_INTR_DISABLE,
            .timer_sel      = LEDC_TIMER_0,
            .duty           = 0, // starting brighness 0
            .hpoint         = 0,
            .flags = {.output_invert = false},
        };
        ESP_ERROR_CHECK(
            ledc_channel_config(&ledc_channel));
    }
    ESP_LOGI(TAG, "Done");
}

/** De-initialize this Output module. */
void OutputLed::stop() {
    for (uint8_t i = 0; i < LEDC_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }

        const StaticConfig &config = CONFIG[i];

        ESP_ERROR_CHECK(
            ledc_stop(config.mode, config.channel, 0)); // idle level 0
    }
}

/** Outputs the processed signals to the HW */
void OutputLed::step(const rcProc::StepInfo& info) {

    rcSignals::Signals& signals = *(info.signals);

    for (uint8_t i = 0; i < LEDC_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }

        const StaticConfig &config = CONFIG[i];

        // 100% duty is 8191
        // So we just multiply by 8 and cut off at max duty.
        int duty = 0;
        if (signals[types[i]] >= RCSIGNAL_NEUTRAL) {
            duty = signals[types[i]] * 8;
        } else {
            duty = 0;
        }
        if (duty > LEDC_MAX_DUTY) {
            duty = LEDC_MAX_DUTY;
        }

        ESP_ERROR_CHECK(
            ledc_set_duty(config.mode, config.channel, duty));
        ESP_ERROR_CHECK(
            ledc_update_duty(config.mode, config.channel));
    }
}


} // namespace

