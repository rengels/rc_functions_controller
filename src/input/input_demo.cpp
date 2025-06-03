/**
 *  Implementation of the demo input functionality.
 *
 *  @file
*/

#include "input.h"
#include "signals.h"
#include "input_demo.h"

#include <array>

using namespace rcSignals;

namespace rcInput {

/** This struct is used for the demo script. */
struct DemoStep {
    TimeMs time;
    SignalType type;
    RcSignal value;
};

/** Script for a Truck demo
 *
 *  With lights, trailer, horn and a lot of other features.
 *  Auto-indicator, auto gear, auto ignition
 */
static DemoStep scriptTruck[] = {
    // init
    {0, SignalType::ST_YAW, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_SPEED, 0},
    {0, SignalType::ST_TRAILER_SWITCH, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_HORN, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_LOWBEAM, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_HIGHBEAM, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_CABIN, RCSIGNAL_MAX},
    {0, SignalType::ST_ROOF, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_SIDE, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_BEACON, RCSIGNAL_NEUTRAL},

    { 1100, SignalType::ST_SPEED, -50}, // slowly backwards
    { 1100, SignalType::ST_SIDE, RCSIGNAL_MAX},
    { 5000, SignalType::ST_SPEED, 0}, // stop
    { 5000, SignalType::ST_TRAILER_SWITCH, RCSIGNAL_MAX},
    { 5000, SignalType::ST_CABIN, RCSIGNAL_NEUTRAL},
    { 8000, SignalType::ST_SPEED, 100}, // slowly forward
    { 8000, SignalType::ST_CABIN, RCSIGNAL_NEUTRAL},
    { 8000, SignalType::ST_LOWBEAM, RCSIGNAL_MAX},
    { 8000, SignalType::ST_ROOF, RCSIGNAL_MAX},
    {10500, SignalType::ST_YAW, RCSIGNAL_MAX},
    {14500, SignalType::ST_YAW, RCSIGNAL_NEUTRAL},
    {15000, SignalType::ST_SPEED, 1000}, // full speed
    {15000, SignalType::ST_HORN, RCSIGNAL_MAX},
    {16000, SignalType::ST_HORN, RCSIGNAL_NEUTRAL},
    {16000, SignalType::ST_HIGHBEAM, RCSIGNAL_MAX},
    {18200, SignalType::ST_BEACON, RCSIGNAL_MAX},
    {22000, SignalType::ST_SPEED, 0}, // Braking
    {22000, SignalType::ST_HIGHBEAM, RCSIGNAL_MAX},

    {30000, SignalType::ST_NONE, 0} // stop marker
};


/** Script for a Train demo
 */
static DemoStep scriptTrain[] = {
    // init
    {0, SignalType::ST_THROTTLE, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_BRAKE,    RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_IGNITION, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_TRAILER_SWITCH, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_HORN, RCSIGNAL_NEUTRAL},

    {    0, SignalType::ST_IGNITION, RCSIGNAL_MAX},
    { 3000, SignalType::ST_THROTTLE, -250}, // slowly backwards
    { 6500, SignalType::ST_THROTTLE, 0}, // stop
    { 7000, SignalType::ST_BRAKE,    RCSIGNAL_MAX}, // Braking
    { 8000, SignalType::ST_TRAILER_SWITCH, RCSIGNAL_MAX},
    { 8500, SignalType::ST_THROTTLE, 250}, // slowly forward
    { 8500, SignalType::ST_BRAKE,    RCSIGNAL_NEUTRAL},

    {12000, SignalType::ST_HORN, RCSIGNAL_MAX},
    {12500, SignalType::ST_HORN, RCSIGNAL_NEUTRAL},
    {13000, SignalType::ST_HORN, RCSIGNAL_MAX},
    {14500, SignalType::ST_HORN, RCSIGNAL_NEUTRAL},

    {16000, SignalType::ST_THROTTLE, 1000}, // full speed
    {30000, SignalType::ST_THROTTLE, 0}, // Braking
    {30000, SignalType::ST_BRAKE,    RCSIGNAL_MAX}, // Braking

    {41000, SignalType::ST_IGNITION, RCSIGNAL_NEUTRAL},

    {42000, SignalType::ST_NONE, 0} // stop marker
};


/** Script for a Car demo
 */
static DemoStep scriptCar[] = {
    // init
    {0, SignalType::ST_THROTTLE, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_BRAKE,    RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_IGNITION, RCSIGNAL_NEUTRAL},
    {0, SignalType::ST_GEAR, 0},

    {    0, SignalType::ST_IGNITION, RCSIGNAL_MAX},
    // rev
    { 3000, SignalType::ST_THROTTLE, RCSIGNAL_MAX},
    { 3500, SignalType::ST_THROTTLE, 0}, // stop
    { 5800, SignalType::ST_GEAR, 1},
    { 6000, SignalType::ST_THROTTLE, RCSIGNAL_MAX},
    { 7800, SignalType::ST_GEAR, 2},
    { 9800, SignalType::ST_GEAR, 3},
    {13000, SignalType::ST_THROTTLE, RCSIGNAL_NEUTRAL},
    {14000, SignalType::ST_BRAKE, 500},
    {14800, SignalType::ST_GEAR, 3},
    {20800, SignalType::ST_GEAR, 2},
    {30800, SignalType::ST_GEAR, 1},
    {31000, SignalType::ST_BRAKE, RCSIGNAL_MAX},
    {33800, SignalType::ST_GEAR, 0},
    {34000, SignalType::ST_IGNITION, RCSIGNAL_NEUTRAL},

    {35000, SignalType::ST_NONE, 0} // stop marker
};


/** Script for the simple demo
 */
static DemoStep scriptSimple[] = {
    // init
    {0, SignalType::ST_THROTTLE, RCSIGNAL_NEUTRAL},

    {    0, SignalType::ST_THROTTLE, RCSIGNAL_MAX},
    {20000, SignalType::ST_THROTTLE, RCSIGNAL_NEUTRAL},
    {40000, SignalType::ST_NONE, 0} // stop marker
};

static std::array<DemoStep*, 4> scripts = {scriptTruck, scriptTrain, scriptCar, scriptSimple};

/** Constructor */
InputDemo::InputDemo(DemoType typeVal) :
    myTime(0u),
    currentStep(0u),
    scriptType(typeVal) {
}

InputDemo::~InputDemo() {
    stop();
}

/** Just resets all values so that the demo can start from the top.
 */
void InputDemo::start() {
    mySignals.reset();
    myTime = 0u;
    currentStep = 0u;
}

/** Undo everything from the start() function.
 */
void InputDemo::stop() {
}

/** Step through the script and set the input signals
 *
 *  Call this function around once every 20ms.
 */
void InputDemo::step(const rcProc::StepInfo& info) {

    Signals* const signals = info.signals;
    if (static_cast<uint8_t>(scriptType) >= scripts.size()) {
        scriptType = DemoType::TRUCK;
    }
    auto script = scripts[static_cast<uint8_t>(scriptType)];

    myTime += info.deltaMs;

    while ((script[currentStep].type != SignalType::ST_NONE) &&
           (script[currentStep].time <= myTime)) {
        mySignals[script[currentStep].type] = script[currentStep].value;
        currentStep++;
    }

    if ((script[currentStep].type == SignalType::ST_NONE) &&
        (script[currentStep].time <= myTime)) {
        start(); // reset everything
    }

    // copy all signals (unless previously set)
    for (uint8_t i = 0; i < Signals::NUM_SIGNALS; i++) {
        SignalType type = static_cast<SignalType>(i);
        if ((*signals)[type] == RCSIGNAL_INVALID) {
            (*signals)[type] = mySignals[type];
        }
    }
}

} // namespace

