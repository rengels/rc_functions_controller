/**
 *  This file contains definition for the ProcCombine class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_COMBINE_H_
#define _RC_PROC_COMBINE_H_

#include "proc.h"

#include <cstdint>
#include <array>

namespace rcProc {

/** Combine effect proc
 *
 *  This proc combined two input signals in different ways.
 *
 *  - _in1_ stands for input signal 1
 *  - _in2_ stands for input signal 2
 *  - outputs are not cropped MAX/MIN
 *
 *  For logical operations an input signal is considered _true_
 *  if it's at least half way between neutral and max.
 *
 *  | name     | output 1          | output 2 | note       |
 *  |----------|-------------------|----------|------------|
 *  | and      | (in1 and in2)     | not (in1 and in2) |  |
 *  | or       | (in1 or in2)      | not (in1 or in2) |  |
 *  | sub      | in1 - in2         | in1 + in2 | can be used for differential trust |
 *  | mul      | in1 * in2 / 1000  | abs(in1 * in2 / 1000) |  |
 *  | switch   | in1 (if in2 true) else 0 | in1 (if in2 true) else invalid | |
 *  | either   | in2 (if in1 invalid) else 0| in2 (if in1 invalid) else invalid | |
 *
 */
class ProcCombine: public Proc {
    public:
        enum class Function : uint8_t {
            F_AND,
            F_OR,
            F_SUB,
            F_MUL,
            F_SWITCH,
            F_EITHER,
        };
    private:

        Function func;  ///< The type of function
        std::array<rcSignals::SignalType, 2> inTypes;  ///< The signal type for the input value
        std::array<rcSignals::SignalType, 2> outTypes;  ///< The signal type for the output value

    public:
        ProcCombine();
        ProcCombine(
            rcSignals::SignalType inType1,
            rcSignals::SignalType inType2,
            rcSignals::SignalType outType1,
            rcSignals::SignalType outType2,
            Function funcValue);

        virtual void step(const StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcCombine&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcCombine&);
};

} // namespace

#endif // _RC_PROC_COMBINE_H_
