/**
 *  Implementation of the pin input functionality.
 *
 *  @file
*/

#include "input_pin.h"
#include "signals.h"

#include <driver/gpio.h>

using namespace rcSignals;

namespace rcInput {

/** Constructor */
InputPin::InputPin():
    types{SignalType::ST_TRAILER_SWITCH,
        SignalType::ST_NONE,} {
}

InputPin::~InputPin() {
    stop();
}

/** Reserve resources for Pin input, activate the ISR
 *
 */
void InputPin::start() {
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }

        gpio_set_direction(PINS[i], GPIO_MODE_INPUT);
    }
}

/** Undo everything from the start() function.
 *
 *  Note: we can leave the pins at input.
 */
void InputPin::stop() {
}

/** Receive the raw signals from Pin input.
 *
 *  Check for each channel, if the recording has happened.
 *  In that case we will convert the times to signal values and
 *  restart the channel.
 *
 *  Call this function around once every 20ms.
 */
void InputPin::step(const rcProc::StepInfo& info) {
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }

        info.signals->safeSet(
            types[i],
            (gpio_get_level(PINS[i]) > 0) ? RCSIGNAL_MAX : RCSIGNAL_NEUTRAL);
    }

}

} // namespace

