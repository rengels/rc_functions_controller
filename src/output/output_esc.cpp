/**
 *  Implementation of the output via MCPWM (for ESC for motors)
 *
 *  @file
*/

#include "output_esc.h"
#include "signals.h"

#include <cstdint>
#include <cmath>  // for min/max

#include <driver/mcpwm_prelude.h>
#include <driver/gpio.h>
#include <esp_err.h>

using namespace rcSignals;

namespace rcOutput {

int16_t OutputEsc::signalToPwm(const rcSignals::RcSignal signal) const {

    rcSignals::RcSignal absSignal = std::abs(signal);

    uint32_t result;
    if (signal == RCSIGNAL_INVALID) {
        result = 0u;

    } else if (absSignal > deadZone) {
        if (absSignal < RCSIGNAL_MAX) {
            result = absSignal;
        } else {
            result = RCSIGNAL_MAX;
        }

    } else {
        result = 0u;
    }

    return result;
}


OutputEsc::OutputEsc() {
    types = {
        SignalType::ST_WINCH,
        SignalType::ST_NONE,
        SignalType::ST_NONE,
    };
    pins = {
            GPIO_NUM_12,
            GPIO_NUM_22,
            GPIO_NUM_32
        };
    pins2 = {
            GPIO_NUM_13,
            GPIO_NUM_23,
            GPIO_NUM_33
    };
    freqTypes = {FreqType::KHZ10};
    deadZone = 100u;
}


void OutputEsc::start() {

    OutputPwm::start();

    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }
        gpio_set_direction(pins2[i], GPIO_MODE_OUTPUT);
        gpio_set_level(pins2[i], 1);
    }
}

void OutputEsc::stop() {

    // set the pins back to zero
    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }
        gpio_set_level(pins2[i], 0);

        if (handleCmpr[i] != nullptr) {
            mcpwm_comparator_set_compare_value(
                handleCmpr[i],
                0);
        }
    }

    OutputPwm::stop();
}

void OutputEsc::stepSlow(const rcProc::StepInfo& info, uint16_t stepIncrement) {

    static uint32_t step = 0u;
    step += stepIncrement;
    if (step >= RCSIGNAL_MAX) {
        step = 0u;
    }

    uint32_t period = 20000u; // 20ms, a cycle period of the main task

    ESP_ERROR_CHECK(
        mcpwm_timer_set_period(handleTimer, period));

    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }
        if (handleCmpr[i] != nullptr) {
            const RcSignal signal = (*(info.signals))[types[i]];

            uint32_t value = signalToPwm(signal);
            if (value > step + stepIncrement) {
                value = period;  // full step
            } else if (value > step) {
                value = (value - step) * period / stepIncrement;  // partial step
            } else {
                value = 0u;  // no step
            }

            if (signal >= 0) {
                gpio_set_level(pins2[i], 0);
                ESP_ERROR_CHECK(
                    mcpwm_comparator_set_compare_value(
                        handleCmpr[i],
                        value));

            } else {
                gpio_set_level(pins2[i], 1);
                ESP_ERROR_CHECK(
                    mcpwm_comparator_set_compare_value(
                        handleCmpr[i],
                        (period) - value));
            }
        }
    }
}


void OutputEsc::stepFast(const rcProc::StepInfo& info, uint32_t period) {

    if (handleTimer != nullptr) {
        ESP_ERROR_CHECK(
            mcpwm_timer_set_period(handleTimer, period));
    }

    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }
        if (handleCmpr[i] != nullptr) {
            const RcSignal signal = (*(info.signals))[types[i]];

            uint32_t value = std::min(
                signalToPwm(signal) * period / RCSIGNAL_MAX,
                period);

            if (signal >= 0) {
                gpio_set_level(pins2[i], 0);
                ESP_ERROR_CHECK(
                    mcpwm_comparator_set_compare_value(
                        handleCmpr[i],
                        value));

            } else {
                gpio_set_level(pins2[i], 1);
                ESP_ERROR_CHECK(
                    mcpwm_comparator_set_compare_value(
                        handleCmpr[i],
                        period - value));
            }
        }
    }
}


void OutputEsc::step(const rcProc::StepInfo& info) {

    // -- find the greatest FreqType for the output signals;
    FreqType maxFreqType = FreqType::KHZ10;
    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }
        if (handleCmpr[i] != nullptr) {

            const RcSignal signal = (*(info.signals))[types[i]];
            const int16_t value = signalToPwm(signal);
            const uint8_t slot = std::min(
                static_cast<uint8_t>(
                    static_cast<uint32_t>(value) * NUM_FREQ_SLOTS / RCSIGNAL_MAX),
                static_cast<uint8_t>(freqTypes.size() - 1u));
            const FreqType freqType = freqTypes[slot];

            if ((value > 0) && (freqType > maxFreqType)) {
                maxFreqType = freqType;
            }
        }
    }

    // -- call the actual step function
    switch (maxFreqType) {
    case FreqType::KHZ10:
        stepFast(info, 100u);
        break;
    case FreqType::KHZ5:
        stepFast(info, 500u);
        break;
    case FreqType::KHZ1:
        stepFast(info, 1000u);
        break;
    case FreqType::HZ100:
        stepFast(info, 10000u);
        break;
    case FreqType::HZ10:
        stepSlow(info, 200u);
        break;
    case FreqType::HZ5:
        stepSlow(info, 100u);
        break;
    default:
        ; // do nothing
    }
}


} // namespace

