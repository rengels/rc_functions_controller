/**
 *  This file contains definition for the ProcMissfire class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#include "proc_misfire.h"
#include "signals.h"

#ifdef ARDUINO
#include <esp_random.h>
#else
#include <stdlib.h>
#endif

#include <cstdint>

using namespace::rcSignals;

namespace rcProc {

ProcMisfire::ProcMisfire():
    outMisfireType(SignalType::ST_AUX1),
    misfireChance(100u)
{}

void ProcMisfire::step(const StepInfo& info) {

    Signals* const signals = info.signals;

    RcSignal rpm = signals->get(SignalType::ST_RPM, RCSIGNAL_NEUTRAL);

#ifdef ARDUINO
    uint32_t rndNr = esp_random();
#else
    int rndNr = rand();
#endif

    // missfire
    // uint8_t missfire TODO: low RPM
    if ((rndNr & 0xff) < misfireChance) {
        (*signals)[SignalType::ST_RPM] = 0;

        if (outMisfireType != SignalType::ST_NONE) {
            (*signals)[outMisfireType] = RCSIGNAL_MAX;
        }
    }
}

} // namespace

