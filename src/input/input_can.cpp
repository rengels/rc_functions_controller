/**
 *  Implementation of the CAN input functionality.
 *
 *  @file
*/

#include "input_can.h"
#include "signals.h"

using namespace rcSignals;
using namespace rcProc;

namespace rcInput {

InputCan::InputCan() : canId(0x35Du) {
}

void InputCan::start() {
    can.start(this);
}

void InputCan::stop() {
    can.stop(this);
}

void InputCan::step(const StepInfo& info) {
    can.step(this, info);
}

} // namespace

