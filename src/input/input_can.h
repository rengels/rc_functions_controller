/**
 *  Implementation of the CAN input functionality.
 *
 *  @file
*/

#include "input.h"
#include "signals.h"
#include "../output/output_can.h"

namespace rcInput {

/** This class reads CAN input messages
 *
 *  This class reads CAN input messages with the
 *  given identifier.
 *  The CAN message can contain one or two signal values
 *  including ID.
 *
 *  The expected format for the message is like this:
 *
 *  byte 0 | byte 1 | byte 2 | byte 3 | byte 4 | byte 5 | byte 6
 *  -------|--------|--------|--------|--------|--------|--------
 *  signal ID | MSB | LSB | sig. ID. 2 (optional) | MSB (optional) | LSB (optional)
 *
 *  The signal values will be buffered for one cycle (20ms), so that
 *  two function controllers can communicate without signal loss through
 *  race conditions.
 *
 */
class InputCan : public Input {

private:
    /** Ref counter object that ensures that the can driver is correctly
     *  initialized. */
    rcOutput::CanDriverWrapper can;

    rcOutput::CanId canId;  ///< the CAN message ID we are looking for

public:
    InputCan();
    virtual ~InputCan() = default;

    virtual void start() override;
    virtual void stop() override;

    /** Read all available messages from CAN
     *
     *  Call this function around once every 20ms.
     */
    virtual void step(const rcProc::StepInfo& info) override;

    friend rcOutput::CanDriverWrapper;
    friend SimpleOutStream& operator<<(::SimpleOutStream& out, const InputCan&);
    friend SimpleInStream& operator>>(::SimpleInStream& in, InputCan&);
};

} // namespace

