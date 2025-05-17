/**
 *  Implementation of the sbus input functionality.
 *
 *  @file
*/

#include "input_sbus.h"
#include "signals.h"

#include <hal/gpio_types.h> // for gpio_num_t
#include <driver/uart.h>
#include <esp_err.h>
#include <esp_log.h>

const static char *TAG = "inSBUS";

using namespace rcSignals;

namespace rcInput {


InputSbus::InputSbus():
    pin(GPIO_NUM_36),
    fast(false),
    inverted(true),
    initialized(false),
    lastSignals{RCSIGNAL_INVALID},
    types{SignalType::ST_ROLL,
        SignalType::ST_PITCH,
        SignalType::ST_YAW,
        SignalType::ST_AUX1,
        SignalType::ST_THROTTLE,
        SignalType::ST_AUX2,
        SignalType::ST_NONE,
        SignalType::ST_NONE},
    notUpdatedCtr{0U} {
}

InputSbus::~InputSbus() {
    stop();
}

/** Reserve resources for SRXL input, activate the ISR
 */
void InputSbus::start() {

    msgLen = 0;
    lastSignals = {RCSIGNAL_INVALID};
    notUpdatedCtr = {0U};

    if (!initialized) {
        ESP_LOGI(TAG, "start for PIN: %u, SIGNAL: %u",
            static_cast<uint16_t>(pin), static_cast<uint16_t>(types[0]));

        uart_config_t uart_config = {
            .baud_rate = (fast ? BAUD_RATE_FAST : BAUD_RATE),
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_EVEN,
            .stop_bits = UART_STOP_BITS_2,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
            .source_clk = UART_SCLK_DEFAULT,
            .flags = {
               .backup_before_sleep = false,
            },
        };

        // no tx buffer, queue or interrupts
        ESP_ERROR_CHECK(
            uart_driver_install(UART_NUM,
                1024, 0, 0, NULL, 0));

        ESP_ERROR_CHECK(
            uart_param_config(UART_NUM, &uart_config));

        ESP_ERROR_CHECK(
            uart_set_line_inverse(UART_NUM,
                inverted ? UART_SIGNAL_RXD_INV : UART_SIGNAL_INV_DISABLE));

        ESP_ERROR_CHECK(
            uart_set_pin(UART_NUM,
                UART_PIN_NO_CHANGE, pin,
                UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

        initialized = true;
    }
}

void InputSbus::stop() {
    if (initialized) {
        ESP_ERROR_CHECK(
            uart_driver_delete(UART_NUM));
        initialized = false;
    }
}

int16_t InputSbus::parseForMsg(const uint8_t* data, int16_t dataLen) {

    // check if the buffer has enough data to contain a message
    if (dataLen < MSG_SIZE) {
        return 0;

    } else if (data[0] == HEADER && data[MSG_SIZE] == FOOTER) {

        lastSignals[0]  = (data[1] |
                           ((data[2] << 8) & 0x07FF)) - 1024;
        lastSignals[1]  = ((data[2] >> 3) |
                           ((data[3] << 5) & 0x07FF)) - 1024;
        lastSignals[2]  = ((data[3] >> 6) |
                           (data[4] << 2) |
                           ((data[5] << 10) & 0x07FF)) - 1024;
        lastSignals[3]  = ((data[5] >> 1) |
                           ((data[6] << 7) & 0x07FF)) - 1024;
        lastSignals[4]  = ((data[6] >> 4) |
                           ((data[7] << 4) & 0x07FF)) - 1024;
        lastSignals[5]  = ((data[7] >> 7) |
                           (data[8] << 1) |
                           ((data[9] << 9) & 0x07FF)) - 1024;
        lastSignals[6]  = ((data[9] >> 2) |
                           ((data[10] << 6) & 0x07FF)) - 1024;
        lastSignals[7]  = ((data[10] >> 5) |
                           ((data[11] << 3) & 0x07FF)) - 1024;
        lastSignals[8]  = (data[12] |
                           ((data[13] << 8) & 0x07FF)) - 1024;
        lastSignals[9]  = ((data[13] >> 3) |
                           ((data[14] << 5) & 0x07FF)) - 1024;
        lastSignals[10] = ((data[14] >> 6) |
                           (data[15] << 2) |
                           ((data[16] << 10) & 0x07FF)) - 1024;
        lastSignals[11] = ((data[16] >> 1) |
                           ((data[17] << 7) & 0x07FF)) - 1024;
        lastSignals[12] = ((data[17] >> 4) |
                           ((data[18] << 4) & 0x07FF)) - 1024;
        lastSignals[13] = ((data[18] >> 7) |
                           (data[19] << 1) |
                           ((data[20] << 9) & 0x07FF)) - 1024;
        lastSignals[14] = ((data[20] >> 2) |
                           ((data[21] << 6) & 0x07FF)) - 1024;
        lastSignals[15] = ((data[21] >> 5) |
                           ((data[22] << 3) & 0x07FF)) - 1024;

        lastSignals[16] = (data[23] & 0x01) ? RCSIGNAL_MAX : RCSIGNAL_NEUTRAL;
        lastSignals[17] = (data[24] & 0x01) ? RCSIGNAL_MAX : RCSIGNAL_NEUTRAL;

        notUpdatedCtr = {0};

        return MSG_SIZE;

    } else {
        return 1; // eat one byte away
    }

}

void InputSbus::step(const rcProc::StepInfo& info) {

    if (!initialized) {
        return;
    }

    // -- get new data
    auto len = uart_read_bytes(
        UART_NUM,
        msgBuffer.data() + msgLen,
        msgBuffer.size() - msgLen,
        static_cast<TickType_t>(0));
    msgLen += len;

    // -- look for valid messages
    int16_t parsedChars;
    int16_t offset = 0; // the position at which we look for a msg
    do {
        parsedChars = parseForMsg(msgBuffer.data() + offset, msgLen - offset);
        offset += parsedChars;
    } while (parsedChars > 0);

    // -- clean up msg buffer
    for (int16_t i = 0; i + offset < msgLen ; i++) {
        msgBuffer[i] = msgBuffer[i + offset];
    }
    msgLen -= offset;

    // -- copy last signals, invalidate if not up-to-date
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

