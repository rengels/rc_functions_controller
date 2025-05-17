/**
 *  Implementation of the pwm input functionality.
 *
 *  @file
*/

#include "input_pwm.h"
#include "signals.h"

using namespace rcSignals;

namespace rcInput {

/** Constructor */
InputPwm::InputPwm():
    types{SignalType::ST_HORN,
        SignalType::ST_LI_INDICATOR_LEFT,
        SignalType::ST_THROTTLE,
        SignalType::ST_YAW,
        SignalType::ST_NONE,
        SignalType::ST_NONE,} {
}

InputPwm::~InputPwm() {
    stop();
}

/** Reserve resources for PWM input, activate the ISR
 *
 */
void InputPwm::start() {
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        if (types[i] == SignalType::ST_NONE) {
            continue;
        }

        InputPpm& ppm = ppmModules[i];
        ppm.setConfig({types[i], rcSignals::SignalType::ST_NONE}, PINS[i], 1);
        ppm.start();
    }
}

/** Undo everything from the start() function.
 *
 *  Note: we can leave the pins at input.
 */
void InputPwm::stop() {
    for (auto& ppm : ppmModules) {
        ppm.stop();
    }
}

/** Receive the raw signals from PWM input.
 *
 *  Check for each channel, if the recording has happened.
 *  In that case we will convert the times to signal values and
 *  restart the channel.
 *
 *  Call this function around once every 20ms.
 */
void InputPwm::step(const rcProc::StepInfo& info) {
    for (auto& ppm : ppmModules) {
        ppm.step(info);
    }
}

} // namespace

