/**
 *  This file contains definition for the ProcExpo class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_EXPO_H_
#define _RC_PROC_EXPO_H_

#include "proc.h"

namespace rcProc {

/** Expo effect proc
 *
 *  The _expo_ function is actually just 3nd degree function:
 *
 *      f(x) = bx + dx³
 *      d = 1 - b
 *
 *  @see https://www.rcgroups.com/forums/showthread.php?1310689-What-is-expo-function
 *
 */
class ProcExpo: public Proc {
    private:
        float b; ///< The constant value
        rcSignals::SignalType inType;
        rcSignals::SignalType outType;

    public:
        ProcExpo();

        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcExpo&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcExpo&);
};

} // namespace

#endif // _RC_PROC_EXPO_H_
