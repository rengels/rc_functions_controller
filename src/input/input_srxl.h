/**
 *  Implementation of the serial receiver link protocol (srxl)
 *
 *  @see https://wiki.rc-network.de/wiki/SRXL/Summensignal
 *  @see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html
 *
 *
 *  @file
*/

#ifndef _INPUT_SRXL_H
#define _INPUT_SRXL_H

#include "input.h"
#include "signals.h"

#include <array>

#include <hal/gpio_types.h> // for gpio_num_t
#include <driver/uart.h>

namespace rcInput {

/** This class reads SRXL input signals using an uart module.
 *
 *  Currently we ignore the symbol levels, so it doesn't really
 *  matter if it's a inverse symbol.
 *
 *  Uses:
 *
 *  - uart 2
 *  - gpio pin 36 (as a default)
 *
 */
class InputSrxl : public Input {
    private:

        /** The maximum size of a message.
         *
         *  A frame is:
         *  - 2 bytes header/version
         *  - 16 * 2 bytes servo info
         *  - 2 bytes crc
         */
        static constexpr int16_t MAX_MSG_SIZE = 36;
        static constexpr uint32_t BAUD_RATE = 115200;
        static constexpr uart_port_t UART_NUM = UART_NUM_2;

        static constexpr uint8_t NUM_CHANNELS = 16U;  ///< the maximum number of channels this input proc is handling. Multiplex will send up to 16 channels.
        gpio_num_t pin;  ///< input pin

        bool initialized;

        /** Buffer for read bytes.
         *
         *  In theory we could receive up to 250 bytes in 20ms, but
         *  we assume a "normal" data rate of a frame every 20ms.
         *  So two times MAX_MSG_SIZE should be enough.
         */
        std::array<uint8_t, MAX_MSG_SIZE * 2> msgBuffer;
        int16_t msgLen; // fill cntr of the msgBuffer

        std::array<rcSignals::RcSignal, NUM_CHANNELS> lastSignals;
        std::array<rcSignals::SignalType, NUM_CHANNELS> types;

        /** Count is increased every time the signal was not received. */
        std::array<uint32_t, NUM_CHANNELS> notUpdatedCtr;

        /** After the signal was not received for a number of times,
         *  it get's invalidated.
         */
        static constexpr uint32_t notUpdatedCutoff = 10U;

        /** Parses a message at the given position.
         *
         *  Checks if the data buffer starts with a valid message.
         *  In case of a valid message the lastSignal values will be updated.
         *
         *  @returns the number of characters parsed away.
         */
        int16_t parseForMsg(const uint8_t* data, int16_t dataLen);

    public:
        InputSrxl();
        virtual ~InputSrxl();

        virtual void start() override;
        virtual void stop() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const InputSrxl&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, InputSrxl&);
};


} // namespace

#endif // _INPUT_SRXL_H
