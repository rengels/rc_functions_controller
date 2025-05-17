/**
 *  This file contains definition for the ProcThreshold class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_threshold.h"
#include "signals.h"

using namespace::rcSignals;

namespace rcProc {

ProcThreshold::ProcThreshold():
    highThreshold(500),
    lowThreshold(100),
    inType(SignalType::ST_AUX1),
    outType(SignalType::ST_VCC),
    triggered(false)
{}

void ProcThreshold::start() {
    triggered = false;
}

void ProcThreshold::step(const StepInfo& info) {

    Signals* const signals = info.signals;
    RcSignal sig = (*signals)[inType];

    if (sig != RCSIGNAL_INVALID) {
        if (triggered) {
            if (sig < lowThreshold) {
                triggered = false;
            }
        } else {
            if (sig > highThreshold) {
                triggered = true;
            }
        }
        (*signals)[outType] = triggered ? RCSIGNAL_MAX : RCSIGNAL_NEUTRAL;
    }
}

} // namespace

