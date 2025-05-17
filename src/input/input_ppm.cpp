/**
 *  Implementation of the ppm input functionality.
 *
 *  @see https://github.com/espressif/esp-idf/blob/v5.2.2/examples/peripherals/rmt/ir_nec_transceiver/main/ir_nec_transceiver_main.c
 *
 *
 *  @file
*/

#include "input_ppm.h"
#include "signals.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/rmt_rx.h>
#include <esp_err.h>
#include <esp_log.h>

const static char *TAG = "inPPM";

using namespace rcSignals;

namespace rcInput {

static bool rmt_rx_done_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    // send the received RMT symbols to the parser task
    xQueueSendFromISR(queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}


InputPpm::InputPpm():
    pin(GPIO_NUM_36),
    numInputs(NUM_CHANNELS),
    rxChannelHandle(nullptr),
    receiveQueue(nullptr),
    lastSignals{RCSIGNAL_INVALID},
    types{SignalType::ST_HORN,
        SignalType::ST_LI_INDICATOR_LEFT,
        SignalType::ST_THROTTLE,
        SignalType::ST_YAW,
        SignalType::ST_NONE,
        SignalType::ST_NONE,
        SignalType::ST_NONE,
        SignalType::ST_NONE,},
    notUpdatedCtr{0U} {

    receiveQueue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    assert(receiveQueue);
}


InputPpm::~InputPpm() {
    vQueueDelete(receiveQueue);
    stop();
}


void InputPpm::start() {
    if (rxChannelHandle == nullptr) {

        ESP_LOGI(TAG, "start for PIN: %u, SIGNAL: %u",
            static_cast<uint16_t>(pin), static_cast<uint16_t>(types[0]));

        rmt_rx_channel_config_t rx_channel_cfg = {
            .gpio_num = pin,
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = RESOLUTION_HZ,
            .mem_block_symbols = 64, // must be at least 64
        };
        ESP_ERROR_CHECK(
            rmt_new_rx_channel(&rx_channel_cfg, &rxChannelHandle));

        rmt_rx_event_callbacks_t cbs = {
            .on_recv_done = rmt_rx_done_callback,
        };
        ESP_ERROR_CHECK(
            rmt_rx_register_event_callbacks(rxChannelHandle, &cbs, receiveQueue));

        receiveConfig = {
            .signal_range_min_ns =  1250, // smallest "min_ns" that I can set here
            .signal_range_max_ns = 5000000, // 5ms, longer signals indicate the gap
        };

        ESP_ERROR_CHECK(
            rmt_enable(rxChannelHandle));

        // ready to receive
        ESP_ERROR_CHECK(
            rmt_receive(
                rxChannelHandle,
                rawSymbols,
                sizeof(rawSymbols),
                &receiveConfig));
    }
}


void InputPpm::stop() {
    if (rxChannelHandle != nullptr) {
        ESP_ERROR_CHECK(
            rmt_disable(rxChannelHandle));
        ESP_ERROR_CHECK(
            rmt_del_channel(rxChannelHandle));
        rxChannelHandle = nullptr;
    }
}


void InputPpm::step(const rcProc::StepInfo& info) {

    // wait for RX done signal
    rmt_rx_done_event_data_t rx_data;
    if (xQueueReceive(receiveQueue, &rx_data, 0) == pdPASS) {

        if (rx_data.num_symbols != numInputs) {
            // oho. something went bad.

        } else {
            // decode:
            for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
                if (rx_data.num_symbols < i) {
                    continue;
                }
                // Note: we ignore level0 and level1 of rx_data
                if (types[i] == SignalType::ST_NONE) {
                    continue;
                }

                const rmt_symbol_word_t& symbol = rx_data.received_symbols[i];

                RcSignal signal = symbol.duration0;
                signal -= 1500 * 2; // resolution .5us

                // Only take valid signals!
                if (signal > -1500 && signal < 1500) {
                    lastSignals[i] = signal;
                    notUpdatedCtr[i] = 0;
                }
            }
        }

        // start receive again
        ESP_ERROR_CHECK(
            rmt_receive(
                rxChannelHandle,
                rawSymbols,
                sizeof(rawSymbols),
                &receiveConfig));
    }

    // copy last signals, invalidate if not up-to-date
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        notUpdatedCtr[i]++;
        if (notUpdatedCtr[i] >= notUpdatedCutoff) {
            lastSignals[i] = RCSIGNAL_INVALID;
        }

        if (types[i] != SignalType::ST_NONE) {
            info.signals->safeSet(types[i], lastSignals[i]);
        }
    }
}

} // namespace

