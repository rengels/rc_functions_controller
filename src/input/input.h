/**
 *  This file contains definition for the Input class
 *  with the rc_functions_controller project.
 *
 *  Possibly inputs should be:
 *
 *  SBUS
 *  IBUS
 *  PPM
 *  SUMD
 *  PWM
 *  ADC aka battery voltage
 *
 *  @file
*/

#ifndef _RC_INPUT_H_
#define _RC_INPUT_H_

#include <proc.h>

/** Namespace for Proc classes that receive input (mostly via HW modules) */
namespace rcInput {

/** Abstract base class for Input components.
 *
 *  RCInput classes are *Proc* responsible for filling out
 *  a rawSignals structure.
 *
 *  Signals that are not read should be set to RCSIGNAL_INVALID
 */
class Input : public rcProc::Proc {
public:
    virtual ~Input() {}
};

} // namespace

#endif // _RC_INPUT_H_
