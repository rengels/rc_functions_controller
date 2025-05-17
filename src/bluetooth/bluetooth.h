/** Outside interface for bluetooth component of rc functions controller
 *
 *  @file
 *
 */

#ifndef _RC_BLUETOOTH_
#define _RC_BLUETOOTH_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A C style span for usage in the queues. */
typedef struct {
    /** Pointer to dynamically allocated buffer.
     *
     *  Make sure to free after use.
     */
    uint8_t* data;

    /** Filled size of the data. */
    size_t len;
} QueueByteBuffer;

/** Initializes and starts the bluetooth component (in a thread) */
void btStart(void);

/** Stops the bluetooth component */
void btStop(void);


/** Proposed size for the buffer containing signals in bytes.
 */
#define BUFFER_SIGNAL_SIZE 140

/** Proposed size for the buffer containing configuration in bytes */
#define BUFFER_CONFIG_SIZE 500

/** Size for the buffer containing audio sample commands in bytes */
#define BUFFER_AUDIO_SIZE 150

/** Proposed size for the buffer containing audio list in bytes */
#define BUFFER_AUDIO_LIST_SIZE 150


/** Message queue containing a byte buffer with the latest signals. */
extern QueueHandle_t queueOutSignals;

/** Message queue containing a byte buffer signals received via bluetooth. */
extern QueueHandle_t queueInSignals;

/** Message queue containing the current (proc) configuration. */
extern QueueHandle_t queueOutConfig;

/** Message queue containing (proc) configuration received via bluetooth. */
extern QueueHandle_t queueInConfig;

/** Message queue containing audio data received via bluetooth. */
extern QueueHandle_t queueInAudio;

/** Message queue containing audio list to send via bluetooth. */
extern QueueHandle_t queueOutAudioList;

#ifdef __cplusplus
}
#endif

#endif // _RC_BLUETOOTH_

