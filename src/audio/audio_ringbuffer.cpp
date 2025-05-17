/* RC engine sound & light controller for Arduino ESP32.
 *
 * Contains the class handling the audio sample buffer.
 *
 */

#include "audio_ringbuffer.h"
#include <cstdint>
#include <array>

static rcAudio::AudioRingbuffer singletonRingbuffer;

namespace rcAudio {

rcProc::SamplesInterval AudioRingbuffer::getEmptyBlocks() {
    rcProc::SamplesInterval result = {
        &(buffer[indexEmpty * BLOCK_SIZE]),
        &(buffer[indexEmpty * BLOCK_SIZE]),
    };

    while ((blockStatus[indexEmpty] == BlockStatus::EMPTY) &&
        (indexEmpty < NUM_BLOCKS)) {
        result.last += BLOCK_SIZE;
        blockStatus[indexEmpty] = BlockStatus::WRITING;
        indexEmpty++;
    };

    // zero out the blocks
    for (auto i = result.first; i < result.last; i++) {
        i->channel1 = 0;
        i->channel2 = 0;
    }

    // wrap-around
    if (indexEmpty >= NUM_BLOCKS) {
        indexEmpty = 0;
    }

    return result;
}

void rcAudio::AudioRingbuffer::setBlocksFull(rcProc::SamplesInterval iv) {
    int32_t startIndex = (iv.first - &(buffer[0])) / BLOCK_SIZE;
    int32_t endIndex = (iv.last - &(buffer[0])) / BLOCK_SIZE;

    if (startIndex >= 0 && startIndex < NUM_BLOCKS) {
        for (int32_t i = startIndex; i < endIndex && i < NUM_BLOCKS; i++) {
            blockStatus[i] = BlockStatus::FULL;
        }
    }
}

rcProc::SamplesInterval rcAudio::AudioRingbuffer::getFullBlocks() {
    rcProc::SamplesInterval result = {
        &(buffer[indexFull * BLOCK_SIZE]),
        &(buffer[indexFull * BLOCK_SIZE]),
    };

    if ((blockStatus[indexFull] == BlockStatus::FULL) &&
        (indexFull < NUM_BLOCKS)) {
        result.last += BLOCK_SIZE;
        blockStatus[indexFull] = BlockStatus::READING;
        indexFull++;
    };

    // wrap-around
    if (indexFull >= NUM_BLOCKS) {
        indexFull = 0;
    }

    return result;
}

void rcAudio::AudioRingbuffer::setBlocksEmpty(rcProc::SamplesInterval iv) {

    int32_t startIndex = (iv.first - &(buffer[0])) / BLOCK_SIZE;
    int32_t endIndex = (iv.last - &(buffer[0])) / BLOCK_SIZE;

    if (startIndex >= 0 && startIndex < NUM_BLOCKS) {
        for (int32_t i = startIndex; i < endIndex && i < NUM_BLOCKS; i++) {
            blockStatus[i] = BlockStatus::EMPTY;
        }
    }
}

uint8_t rcAudio::AudioRingbuffer::getNumEmpty() const {
    uint8_t num = 0;
    for (const auto& status : blockStatus) {
        if (status == BlockStatus::EMPTY) {
            num++;
        }
    }
    return num;
}

uint8_t rcAudio::AudioRingbuffer::getNumFull() const {
    uint8_t num = 0;
    for (const auto& status : blockStatus) {
        if (status == BlockStatus::FULL) {
            num++;
        }
    }
    return num;
}

AudioRingbuffer& getRingbuffer() {
    return singletonRingbuffer;
}

} // namespace

