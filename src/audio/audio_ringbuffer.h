/* RC engine sound & light controller for Arduino ESP32.
 *
 * Contains the class handling the audio sample buffer.
 *
 */

#ifndef _AUDIO_RINGBUFFER_H_
#define _AUDIO_RINGBUFFER_H_

#include "proc.h"
#include <cstdint>
#include <array>

namespace rcAudio {

/** This class represents a ring-buffer with audio samples.
 *
 *  Since we are using DMA the ringbuffer consists of several
 *  memory blocks being outputted via continous DAC conversion.
 *
 *  Written can be done in a batch.
 *  You will need to write at most two blocks of buffer.
 *
 *  - current index to end
 *  - start to final index.
 *
 *  Note: this should be thread-safe if read and write are done
 *   in different threads.
 */
class AudioRingbuffer {

public:
    static constexpr int32_t BLOCK_SIZE = 256; ///< memory block size (number of rcProc::AudioSamples)
    static constexpr int32_t NUM_BLOCKS = 7; ///< number of memory blocks (we need enough for at least 20ms)

private:
    /** Status of the different memory blocks.
     */
    enum class BlockStatus : uint8_t {
        EMPTY,  ///< Memory block is currently empty
        WRITING,  ///< Memory block is reserved for writing
        FULL,  ///< Memory block has been written
        READING  ///< Memory block has been given to DMA
    };

    std::array<rcProc::AudioSample, NUM_BLOCKS * BLOCK_SIZE> buffer;
    std::array<BlockStatus, NUM_BLOCKS> blockStatus;

    uint8_t indexEmpty;  ///< The index to the first empty block
    uint8_t indexFull;  ///< The index to the first full block

public:
    /** You might not want to create a ringbuffer yourself
     *  Instead use the getRingbuffer function.
     */
    AudioRingbuffer():
          indexEmpty(0),
          indexFull(0) {
        for (auto& status : blockStatus) {
            status = BlockStatus::EMPTY;
        }
    }

    /** Returns the next sequence of empty blocks, marking them as WRITING.
     *
     *  The returned blocks will be emptied out (set to 0)
    */
    rcProc::SamplesInterval getEmptyBlocks();

    void setBlocksFull(rcProc::SamplesInterval);

    /** Returns the next full block, marking them as READING.
    */
    rcProc::SamplesInterval getFullBlocks();

    /** Marks the blocks in the given interval as empty.
     *
     *  Call this function after reading has finished.
     */
    void setBlocksEmpty(rcProc::SamplesInterval);

    /** Returns the number of empty blocks.
     */
    uint8_t getNumEmpty() const;

    /** Returns the number of full blocks.
     */
    uint8_t getNumFull() const;
};

/** Returns the ringbuffer used globally. */
AudioRingbuffer& getRingbuffer();

}

#endif // _AUDIO_RINGBUFFER_H_

