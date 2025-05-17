/**
 *  Implementation of a dummy output module.
 *
 *  Might be usefull for testing.
 *
 *  @file
*/

#ifndef _OUTPUT_DUMMY_H_
#define _OUTPUT_DUMMY_H_

#include "output.h"

using namespace rcSignals;

namespace rcOutput {

/** Dummy output. Does nothing.
 */
class OutputDummy : public Output {

public:
    OutputDummy() {}
    virtual ~OutputDummy() {}

    virtual void step(const rcProc::StepInfo& info) override;
};

} // namespace

#endif // _OUTPUT_DUMMY_H_
