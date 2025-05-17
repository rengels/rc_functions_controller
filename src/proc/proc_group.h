/**
 *  This file contains definition for the ProcGroup class
 *  with the Rc_Functions_Controller project.
 *
 *  @file
*/

#ifndef _RC_PROC_GROUP_H_
#define _RC_PROC_GROUP_H_

#include "proc.h"
#include "signals.h"

namespace rcProc {

/** This class doesn't do anything with signals, but is
 *  used in the web UI to group Procs.
 */
class ProcGroup: public Proc {
    private:
        /** The type of group.
         *
         *  This has no internal meaning, but the procs_config.json defines
         *  the following:
         *
         * - Input
         * - Vehicle
         * - Output
         * - Audio
         * - Logic
         * - Lights
         * - Motor
         */
        uint8_t type;

        /** The number of procs considered to be in this group.
         *
         *  This value is only necessary for the UI.
         *  Internally we still consider the procs to be a list, not a tree.
         */
        uint8_t numChilds;

    public:
        ProcGroup(uint8_t typeVal = 0u, uint8_t numChildsVar = 0u):
            type(typeVal),
            numChilds(numChildsVar)
        {}

        virtual ~ProcGroup()
        {}

        virtual void step(const StepInfo&) override
        {}

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const ProcGroup&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, ProcGroup&);
};


} // namespace

#endif // _RC_PROC_GROUP_H_
