/**
 *  Implementation of the output via CAN
 *
 *  @file
*/

#ifndef _OUTPUT_CAN_H_
#define _OUTPUT_CAN_H_

#include "output.h"
#include "signals.h"

#include <atomic>
#include <vector>
#include <driver/gpio.h>

namespace rcOutput {

typedef uint16_t CanId;  ///< CAN message ID
typedef std::array<uint8_t, 8> CanData; ///< CAN message data

/** Representation of a CAN message with receive time.
 *
 *  This struct is used by the CanDriverWrapper to buffer received messages.
 */
struct CanMessage {
    CanId id;
    CanData data;
    uint8_t len;  ///< length of the received messages
    rcSignals::TimeMs recTime;
};


/** This class provides the backend implementation for OutputCan and InputCan.
 *
 *  The DriverWrapper creates and destroys the driver.
 *  It uses an atomic number for ref-counting.
 *
 *  Note: Although an atomic number is used the functions are not reentrant.
 */
class CanDriverWrapper {

private:
    static constexpr uint32_t TWAI_BITRATE = 200000u;
    static constexpr gpio_num_t TX_GPIO_NUM = GPIO_NUM_12;
    static constexpr gpio_num_t RX_GPIO_NUM = GPIO_NUM_14;


    static constexpr rcSignals::TimeMs ALIVE_TIME = 40;  ///< the time that received CAN messages are retained in ms
    static constexpr CanId ID_UNSET = 0xFFFFu;

    /** A list of filter IDs.
     *
     *  Every new CanInput will add to the input filter ID.
     */
    static CanId filterIds;
    static CanId filterMask;

    /** Atomic counter for CanDriverWrapper */
    static std::atomic_int8_t driverRefCount;

    /** A history of received messages at most 40ms old.
     *
     *  The messages are ordered by time, so that the oldest ones
     *  are the last.
     */
    static std::vector<CanMessage> receivedMessages;

    /** Start the driver with the current filterIDs and filterMask.
     *
     *
     */
    void startDriver();

    /** Stop the driver completely. */
    void stopDriver();

    /** Remove messages with an older timestamp than ALIVE_TIME from the input buffer. */
    void removeOldMessages();

    /** Reads all messages from the driver and copies them in the receivedMessages buffer. */
    void readMessages();

public:
    /** Start the can HW driver, update ref count. */
    CanDriverWrapper();

    /** Stop the can driver.
     *
     *  We have a ref-counter running, so we stop the HW can driver
     *  once we destroy the last wrapper.
     */
    ~CanDriverWrapper();

    /** Does the start functionality regarding the CAN HW for the given Proc.
     *
     *  Note: no-one is preventing several "start" calls.
     *
     *  @param[in] proc The Proc that needs the CAN start.
     */
    void start(const rcProc::Proc* proc);
    void stop(const rcProc::Proc* proc);
    void step(const rcProc::Proc* proc, const rcProc::StepInfo& info);
};


/** Implementation of the Output interface via CAN.
 *
 *
 *  The expected format for the message is like this:
 *
 *  byte 0 | byte 1 | byte 2 | byte 3 | byte 4 | byte 5 | byte 6
 *  -------|--------|--------|--------|--------|--------|--------
 *  signal ID | MSB | LSB | sig. ID. 2 (optional) | MSB (optional) | LSB (optional)
 *
 */
class OutputCan : public Output {

private:
    static constexpr uint32_t CAN_QUEUE_LEN = 6;  ///< Number of slots for message queue.
    static constexpr uint8_t CAN_NUM = 2;  ///< Number of slots for sending types. Only 2 fit in a message.

    /** Ref counter object that ensures that the can driver is correctly
     *  initialized. */
    rcOutput::CanDriverWrapper can;

    CanId canId;  ///< the CAN message ID we are sending out.

    /** Output signal types. */
    std::array<rcSignals::SignalType, CAN_NUM> types;


public:
    OutputCan();

    virtual void start() override;
    virtual void stop() override;
    virtual void step(const rcProc::StepInfo& info) override;

    friend CanDriverWrapper;
    friend SimpleOutStream& operator<<(::SimpleOutStream& out, const OutputCan&);
    friend SimpleInStream& operator>>(::SimpleInStream& in, OutputCan&);
};

} // namespace

#endif  // _OUTPUT_CAN_H_
