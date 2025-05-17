/**
 *  Implementation of the sbus protocol
 *
 *  @see https://uwarg-docs.atlassian.net/wiki/spaces/ZP/pages/2238283817/SBUS+Protocol
 *
 *
 *  @file
*/

#ifndef _INPUT_SBUS_H
#define _INPUT_SBUS_H

#include "input.h"
#include "signals.h"

#include <array>

#include <hal/gpio_types.h> // for gpio_num_t
#include <driver/uart.h>

namespace rcInput {

/** This class reads SBUS input signals using an uart module.
 *
 *  Currently we ignore the symbol levels, so it doesn't really
 *  matter if it's a inverse symbol.
 *
 *  Uses:
 *
 *  - uart 2 (same as srxl)
 *  - gpio pin 36 (as a default)
 *
 */
class InputSbus : public Input {
    private:

        /** The maximum size of a message.
         *
         *  25 bytes in a packet
         *
         *  - 1*start byte - 0x0F for data read
         *  - 22 * data byte - 16 RC channels encoded on 22 bytes
         *  - 1 * flag byte - contains 2 data channels(channel 17 & 18 ), with  failsafe flag, and frame_lost flag
         *  - 1 * end byte - 0x00 to indicate an end to the message
         */
        static constexpr int16_t MSG_SIZE = 25;
        static constexpr int32_t BAUD_RATE = 100000;
        static constexpr int32_t BAUD_RATE_FAST = 200000;
        static constexpr uart_port_t UART_NUM = UART_NUM_2;

        static constexpr uint8_t HEADER = 0x0F;
        static constexpr uint8_t FOOTER = 0x00;

        static constexpr uint8_t NUM_CHANNELS = 18U;  ///< the maximum number of channels
        gpio_num_t pin;  ///< input pin
        bool fast; ///< switch between default and fast baud rate
        bool inverted; ///< for Futaba style inverted sbus.

        bool initialized;

        /** Buffer for read bytes.
         *
         *  In theory we could receive up to 250 bytes in 20ms, but
         *  we assume a "normal" data rate of a frame every 20ms.
         *  So two times MSG_SIZE should be enough.
         */
        std::array<uint8_t, MSG_SIZE * 2> msgBuffer;
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
        InputSbus();
        virtual ~InputSbus();

        virtual void start() override;
        virtual void stop() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const InputSbus&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, InputSbus&);
};


} // namespace

#endif // _INPUT_SBUS_H
