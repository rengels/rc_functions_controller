/**
 *  This file contains definition for the RCOutput classes
 *  with the RC_Engine project.
 *
 *  @file
*/

#ifndef _RC_OUTPUT_H_
#define _RC_OUTPUT_H_

#include <proc.h>
#include <array>

/** Namespace containing Proc classes that output signals via the HW */
namespace rcOutput {

/** Abstract base class for rcOutput components.
 *
 *  Outputs usually take signals and export them to HW pins in some
 *  way or another.
 */
class Output : public rcProc::Proc {
    private:
        /** An array to keep track which timer group IDs are available. */
        static std::array<bool, 3> freeTimerGroupIds;

    protected:
        /** Returns the next free timer group ID and reserves it.
         *
         *  Timer groups are a limited resource used by PWM and ESC.
         *  We want to be able to reserve and free them.
         *
         *  @returns A group ID or 255 if no ID is free.
         */
        static uint8_t reserveTimerGroupId();

        /** Free a group Id again. */
        static void freeTimerGroupId(uint8_t id);

    public:
        virtual ~Output() {}
};

} // namespace

#endif // _RC_OUTPUT_H_
