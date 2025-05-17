/**
 *  Implementation of the output via MCPWM (for PWM for servos)
 *
 *  @file
*/

#include "output_pwm.h"
#include "signals.h"

#include <cstdint>

#include <driver/mcpwm_prelude.h>
#include <driver/gpio.h>
#include <esp_err.h>

#include <esp_log.h>
const static char *TAG = "outPWM";

using namespace rcSignals;

namespace rcOutput {

uint32_t OutputPwm::signalToUs(rcSignals::RcSignal signal) const {
    if (signal == RCSIGNAL_INVALID) {
        return 0; // should not produce any signals
    } else {
        return (signal / 2) + 1500;
    }
}

OutputPwm::OutputPwm():
    groupId(255),
    handleTimer(nullptr),
    handleOper {nullptr},
    handleCmpr {nullptr},
    handleGen {nullptr},
    types {
        SignalType::ST_GEAR,
        SignalType::ST_WINCH,
        SignalType::ST_COUPLER,
    },
    pins {
            GPIO_NUM_12,
            GPIO_NUM_13,
            // GPIO_NUM_14
            GPIO_NUM_27
        }
    {
}

void OutputPwm::start() {

    ESP_LOGI(TAG, "start");
    if (handleTimer == nullptr) {
        groupId = reserveTimerGroupId();
        mcpwm_timer_config_t timer_config = {
            .group_id = groupId,
            .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
            .resolution_hz = TIMEBASE_RESOLUTION_HZ,
            .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
            .period_ticks = SERVO_TIMEBASE,  // 20000 ticks, 20ms (50 Hz)
            .intr_priority = 0,
            .flags = {
                .update_period_on_empty = true,
                .update_period_on_sync = false},
        };
        ESP_ERROR_CHECK(
            mcpwm_new_timer(&timer_config, &handleTimer));
    }

    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }

        ESP_LOGI(TAG, "Pin %d", static_cast<int>(i));
        if (handleOper[i] == nullptr) {
            mcpwm_operator_config_t operator_config = {
                .group_id = groupId, // operator must be in the same group to the timer
                .intr_priority = 0,
                .flags = {
                    .update_gen_action_on_tez = true,
                    .update_gen_action_on_tep = false,
                    .update_gen_action_on_sync = false,
                    .update_dead_time_on_tez = false,
                    .update_dead_time_on_tep = false,
                    .update_dead_time_on_sync = false,
                }
            };
            ESP_ERROR_CHECK(
                mcpwm_new_operator(&operator_config, &handleOper[i]));
            ESP_ERROR_CHECK(
                mcpwm_operator_connect_timer(handleOper[i], handleTimer));
        }

        if (handleCmpr[i] == nullptr) {
            mcpwm_comparator_config_t comparator_config = {
                .intr_priority = 0,
                .flags = {
                    .update_cmp_on_tez = true,
                    .update_cmp_on_tep = false,
                    .update_cmp_on_sync = false,
                },
            };
            ESP_ERROR_CHECK(
                mcpwm_new_comparator(handleOper[i], &comparator_config, &handleCmpr[i]));
        }

        if (handleGen[i] == nullptr) {
            mcpwm_generator_config_t generator_config = {
                .gen_gpio_num = pins[i],
                .flags = {
                    .invert_pwm = false,   // Whether to invert the PWM signal (done by GPIO matrix)
                    .io_loop_back = false, // For debug/test, the signal output from the GPIO will be fed to the input path as well
                    .io_od_mode = false,   // Configure the GPIO as open-drain mode
                    .pull_up = false,      // Whether to pull up internally
                    .pull_down = false,    // Whether to pull down internally
                }
            };
            ESP_ERROR_CHECK(
                mcpwm_new_generator(handleOper[i], &generator_config, &handleGen[i]));

            // set the initial compare value (to 0, meaning no signal)
            ESP_ERROR_CHECK(
                mcpwm_comparator_set_compare_value(handleCmpr[i], 0));

            // go high on counter empty
            ESP_ERROR_CHECK(
                mcpwm_generator_set_action_on_timer_event(handleGen[i],
                    MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                        MCPWM_TIMER_EVENT_EMPTY,
                        MCPWM_GEN_ACTION_HIGH)));

            // go low on compare threshold
            ESP_ERROR_CHECK(
                mcpwm_generator_set_action_on_compare_event(handleGen[i],
                    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                        handleCmpr[i],
                        MCPWM_GEN_ACTION_LOW)));
        }
    }

    ESP_LOGI(TAG, "enabling");
    ESP_ERROR_CHECK(
        mcpwm_timer_enable(handleTimer));
    ESP_ERROR_CHECK(
        mcpwm_timer_start_stop(handleTimer, MCPWM_TIMER_START_NO_STOP));
}

void OutputPwm::stop() {
    if (handleTimer != nullptr) {
        ESP_ERROR_CHECK(
            mcpwm_timer_start_stop(handleTimer, MCPWM_TIMER_STOP_EMPTY));
        ESP_ERROR_CHECK(
            mcpwm_timer_disable(handleTimer));
    }

    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (handleGen[i] != nullptr) {
            ESP_ERROR_CHECK(
                mcpwm_del_generator(handleGen[i]));
            handleGen[i] = nullptr;
        }
        if (handleCmpr[i] != nullptr) {
            ESP_ERROR_CHECK(
                mcpwm_del_comparator(handleCmpr[i]));
            handleCmpr[i] = nullptr;
        }

        if (handleOper[i] != nullptr) {
            ESP_ERROR_CHECK(
                mcpwm_del_operator(handleOper[i]));
            handleOper[i] = nullptr;
        }
    }

    if (handleTimer != nullptr) {
        ESP_ERROR_CHECK(
            mcpwm_del_timer(handleTimer));
        handleTimer = nullptr;
        freeTimerGroupId(groupId);
        groupId = 255;
    }
}

void OutputPwm::step(const rcProc::StepInfo& info) {
    for (uint8_t i = 0; i < PWM_NUM; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }
        if (handleCmpr[i] != nullptr) {
            RcSignal signal = (*(info.signals))[types[i]];
            ESP_ERROR_CHECK(
                mcpwm_comparator_set_compare_value(handleCmpr[i],
                    signalToUs(signal)));
        }
    }
}


} // namespace

