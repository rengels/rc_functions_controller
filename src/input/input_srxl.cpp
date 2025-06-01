/**
 *  Implementation of the srxl input functionality.
 *
 *  @see https://wiki.rc-network.de/wiki/SRXL/Summensignal
 *  @see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html
 *
 *  @file
*/

#include "input_srxl.h"
#include "signals.h"

#include <hal/gpio_types.h> // for gpio_num_t
#include <driver/uart.h>
#include <esp_err.h>
#include <esp_log.h>

const static char *TAG = "inSRXL";

using namespace rcSignals;

namespace rcInput {


InputSrxl::InputSrxl():
    pin(GPIO_NUM_36),
    initialized(false),
    lastSignals{RCSIGNAL_INVALID},
    types{SignalType::ST_ROLL,
        SignalType::ST_AUX1,
        SignalType::ST_PITCH,
        SignalType::ST_NONE,
        SignalType::ST_YAW,
        SignalType::ST_AUX2,
        SignalType::ST_THROTTLE,
        SignalType::ST_NONE},
    notUpdatedCtr{0U} {
}

InputSrxl::~InputSrxl() {
    stop();
}

/** Reserve resources for SRXL input, activate the ISR
 */
void InputSrxl::start() {

    msgLen = 0;
    lastSignals = {RCSIGNAL_INVALID};
    notUpdatedCtr = {0U};

    if (!initialized) {
        ESP_LOGI(TAG, "start for PIN: %u, SIGNAL: %u",
            static_cast<uint16_t>(pin), static_cast<uint16_t>(types[0]));

        uart_config_t uart_config = {
            .baud_rate = BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
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
            uart_set_pin(UART_NUM,
                UART_PIN_NO_CHANGE, pin,
                UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

        initialized = true;
    }
}

void InputSrxl::stop() {
    if (initialized) {
        ESP_ERROR_CHECK(
            uart_driver_delete(UART_NUM));
        initialized = false;
    }
}

/** Multiplex CRC checksum algorithm according to
    https://www.multiplex-rc.de/userdata/files/srxl-multiplex-v2.pdf */
uint16_t multiplexCRC16(uint16_t crc, uint8_t value) {

    crc = crc ^ (static_cast<uint16_t>(value) << 8);

    for(uint8_t i = 0; i < 8; i++) {
        if(crc & 0x8000u) {
            crc = (crc << 1) ^ 0x1021u;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

int16_t InputSrxl::parseForMsg(const uint8_t* data, int16_t dataLen) {

    // check if the buffer has enough data to contain a message
    if (dataLen < MAX_MSG_SIZE) {
        return 0;

    // multiplex 12 channel message
    } else if (data[0] == 0xA1 && (data[1] & 0xF0) == 0) {

        // first check crc
        uint16_t crc = 0;
        for (uint8_t i = 0; i < 25; i++) {
            crc = multiplexCRC16(crc, data[i]);
        }

        // if crc valid
        if (((data[25] << 8) | data[26]) == crc) {

            for (uint8_t i = 0; i < 12 && i < NUM_CHANNELS; i += 2) {
                int32_t rawSignal = ((data[i + 1] << 8) | data[i + 2]) & 0x0FFF;
                lastSignals[i]  = (rawSignal - 2048) * 1024 / 1200;
                notUpdatedCtr[i] = 0;
            }
        }
        return 27;

    // spektrum 12 channel message // TODO
    } else if (data[0] == 0xA5 && (data[1] & 0xF0) == 0) {

        for (uint8_t i = 0; i < 7 && i < NUM_CHANNELS; i += 2) {
            int32_t rawSignal = ((data[i + 1] << 8) | data[i + 2]) & 0x0FFF;
            lastSignals[i]  = (rawSignal - 2048) * 1024 / 1200;
            notUpdatedCtr[i] = 0;
        }

        // todo: CRC

        return 18;

    } else {
        return 1; // eat one byte away
    }

}

void InputSrxl::step(const rcProc::StepInfo& info) {

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

    /* for debugging
    static uint16_t cnt = 0;
    cnt++;
    if (cnt > 50) {
        cnt = 0;
        for (uint8_t i = 0; i < msgLen; i++) {
            ESP_LOGI(TAG, " 0x%2x",
                     static_cast<uint16_t>(msgBuffer[i]));
        }
    }
    */

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

