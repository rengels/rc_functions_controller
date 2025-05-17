/**
 *  Implementation of the ppm input functionality.
 *
 *  @see https://github.com/espressif/esp-idf/blob/v5.2.2/examples/peripherals/rmt/ir_nec_transceiver/main/ir_nec_transceiver_main.c
 *
 *
 *  @file
*/

#ifndef _INPUT_PPM_H
#define _INPUT_PPM_H

#include "input.h"
#include "signals.h"

#include <array>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/rmt_rx.h>

namespace rcInput {

/** This class reads PPM input signals from an input pin..
 *
 *  This module can be used multiple times (and in fact it is by InputPwm).
 *
 *  Currently we ignore the symbol levels, so it doesn't really
 *  matter if it's a inverse symbol.
 *
 *  Uses:
 *
 *  - RMT channel
 *  - gpio pin 36 (as a default)
 *
 */
class InputPpm : public Input {
    private:
        static constexpr uint32_t RESOLUTION_HZ = 2000000U; // 2MHz resolution, 1 tick = 0.5us
        static constexpr uint8_t NUM_CHANNELS = 8U;  ///< the maximum number of channels this input proc is handling
        gpio_num_t pin;  ///< input pin

        /** Expected number of inputs.
         *
         *  There must be exactly the expected number of input
         *  pulses or else the whole frame is invalid.
         *  We can't have one error (e.g. at the beginning)
         *  shifting all the inputs around.
         */
        uint8_t numInputs;

        rmt_channel_handle_t rxChannelHandle;  ///< handle for the channel
        rmt_receive_config_t receiveConfig;  ///< will be filled out by start()
        rmt_symbol_word_t rawSymbols[64]; ///< memory for the received symbols. Must be at least 64.

        /** A queue for the communication between the interrupt
         *  and the main thread.
         */
        QueueHandle_t receiveQueue;

        std::array<rcSignals::RcSignal, NUM_CHANNELS> lastSignals;
        std::array<rcSignals::SignalType, NUM_CHANNELS> types;

        /** Count is increased every time the signal was not received. */
        std::array<uint32_t, NUM_CHANNELS> notUpdatedCtr;

        /** After the signal was not received for a number of times,
         *  it get's invalidated.
         */
        static constexpr uint32_t notUpdatedCutoff = 10U;

    public:
        InputPpm();
        virtual ~InputPpm();

        /** Allows InputPwm to set the configuration without being a friend */
        void setConfig(
            std::array<rcSignals::SignalType, NUM_CHANNELS> typesVal,
            gpio_num_t pinVal,
            uint8_t numInputsVal) {

            types = typesVal;
            pin = pinVal;
            numInputs = numInputsVal;
        }

        virtual void start() override;
        virtual void stop() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const InputPpm&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, InputPpm&);
};


} // namespace

#endif // _INPUT_PPM_H
