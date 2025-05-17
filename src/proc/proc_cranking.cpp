/**
 *  This file contains definition for the ProcCranking class
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "proc_cranking.h"
#include "signals.h"

using namespace::rcSignals;

namespace rcProc {

void ProcCranking::step(const StepInfo& info) {

    auto signals = info.signals;
    auto ignition = signals->get(SignalType::ST_IGNITION, RCSIGNAL_NEUTRAL);
    auto rpm = signals->get(SignalType::ST_RPM, RCSIGNAL_NEUTRAL);
    if ((ignition > RCSIGNAL_TRUE) && (rpm < 60)) {
        for (const auto& type : types) {
            if (type != SignalType::ST_NONE) {
                if ((*signals)[type] != RCSIGNAL_INVALID) {
                    (*signals)[type] -= CRANKING_DIM;
                }
            }
        }
    }
}

} // namespace

