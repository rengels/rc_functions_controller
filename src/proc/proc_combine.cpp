/**
 *  This file contains definition for the ProcCombine class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_combine.h"
#include "signals.h"

#include <cmath>  // for abs

using namespace::rcSignals;

namespace rcProc {

ProcCombine::ProcCombine():
    func(Function::F_SUB),
    inTypes{SignalType::ST_THROTTLE, SignalType::ST_YAW},
    outTypes{SignalType::ST_THROTTLE, SignalType::ST_NONE}
{}

ProcCombine::ProcCombine(
    rcSignals::SignalType inType1,
    rcSignals::SignalType inType2,
    rcSignals::SignalType outType1,
    rcSignals::SignalType outType2,
    Function funcValue):

    func(funcValue),
    inTypes{inType1, inType2},
    outTypes{outType1, outType2}
{}

void ProcCombine::step(const StepInfo& info) {

    Signals* const signals = info.signals;

    RcSignal sig1 = (*signals)[inTypes[0]];
    RcSignal sig2 = (*signals)[inTypes[1]];
    bool sig1Bool = sig1 >= RCSIGNAL_TRUE;
    bool sig2Bool = sig2 >= RCSIGNAL_TRUE;
    RcSignal out1 = RCSIGNAL_INVALID;
    RcSignal out2 = RCSIGNAL_INVALID;

    switch (func) {
    case Function::F_AND:
        if ((sig1 != RCSIGNAL_INVALID) &&
            (sig2 != RCSIGNAL_INVALID)) {
            out1 = (sig1Bool && sig2Bool) ? RCSIGNAL_MAX : RCSIGNAL_NEUTRAL;
            out2 = (sig1Bool && sig2Bool) ? RCSIGNAL_NEUTRAL : RCSIGNAL_MAX;
        }
        break;

    case Function::F_OR:
        if ((sig1 != RCSIGNAL_INVALID) &&
            (sig2 != RCSIGNAL_INVALID)) {
            out1 = (sig1Bool || sig2Bool) ? RCSIGNAL_MAX : RCSIGNAL_NEUTRAL;
            out2 = (sig1Bool || sig2Bool) ? RCSIGNAL_NEUTRAL : RCSIGNAL_MAX;
        }
        break;

    case Function::F_SUB:
        if ((sig1 != RCSIGNAL_INVALID) &&
            (sig2 != RCSIGNAL_INVALID)) {

            out1 = sig1 - sig2;
            out2 = sig1 + sig2;
        }
        break;

    case Function::F_MUL:
        if ((sig1 != RCSIGNAL_INVALID) &&
            (sig2 != RCSIGNAL_INVALID)) {

            out1 = static_cast<int32_t>(sig1) * static_cast<int32_t>(sig2) / RCSIGNAL_MAX;
            out2 = std::abs(static_cast<int32_t>(sig1) * static_cast<int32_t>(sig2) / RCSIGNAL_MAX);
        }
        break;

    case Function::F_SWITCH:
        if ((sig1 != RCSIGNAL_INVALID) &&
            (sig2 != RCSIGNAL_INVALID)) {

            out1 = (sig2Bool) ? sig1 : RCSIGNAL_NEUTRAL;
            out2 = (sig2Bool) ? sig1 : RCSIGNAL_INVALID;
        }
        break;

    case Function::F_EITHER:
        if (sig1 != RCSIGNAL_INVALID) {
            out1 = sig1;
            out2 = sig1;

        } else if (sig2 != RCSIGNAL_INVALID) {
            out1 = sig2;
            out2 = sig2;
        } else {
            out1 = RCSIGNAL_NEUTRAL;
            out2 = RCSIGNAL_INVALID;
        }
        break;

    default:
        ; // do nothing
    } // switch

    if (outTypes[0] != rcSignals::SignalType::ST_NONE) {
        (*signals)[outTypes[0]] = out1;
    }
    if (outTypes[1] != rcSignals::SignalType::ST_NONE) {
        (*signals)[outTypes[1]] = out2;
    }
}

} // namespace

