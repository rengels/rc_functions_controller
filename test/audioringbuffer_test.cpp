/** Tests for the AudioRungbuffer class */

#include "audio_ringbuffer.h"
#include <gtest/gtest.h>

using namespace rcAudio;

/** Tests basic functionality for the ringbuffer
 *
 *  Tests
 *
 *  - rcAudio::AudioRingbuffer::getEmptyBlocks()
 *  - rcAudio::AudioRingbuffer::getFullBlocks()
 */
TEST(AudioRingbufferTest, ReadingWriting) {

    rcAudio::AudioRingbuffer buffer;

    // -- empty buffer
    EXPECT_EQ(buffer.NUM_BLOCKS, buffer.getNumEmpty());
    EXPECT_EQ(0, buffer.getNumFull());

    auto iv0 = buffer.getFullBlocks();
    EXPECT_EQ(0,
        iv0.last - iv0.first);

    // -- write all
    auto iv1 = buffer.getEmptyBlocks();
    EXPECT_EQ(buffer.BLOCK_SIZE * buffer.NUM_BLOCKS,
        iv1.last - iv1.first);  // first interval contains all samples

    EXPECT_EQ(0, buffer.getNumEmpty());
    EXPECT_EQ(0, buffer.getNumFull());

    EXPECT_EQ(0, iv1.first->channel1);  // interval is zeroed out
    EXPECT_EQ(0, iv1.first->channel2);  // interval is zeroed out
    (iv1.first)->channel1 = -55;
    (iv1.first)->channel2 = -66;

    // we should not be able to write anything
    iv0 = buffer.getFullBlocks();
    EXPECT_EQ(0,
        iv0.last - iv0.first);
    auto iv2 = buffer.getEmptyBlocks();
    EXPECT_EQ(0,
        iv2.last - iv2.first);

    // mark writing complete
    buffer.setBlocksFull(iv1);
    EXPECT_EQ(0, buffer.getNumEmpty());
    EXPECT_EQ(buffer.NUM_BLOCKS, buffer.getNumFull());

    // -- read one
    iv0 = buffer.getFullBlocks();
    EXPECT_EQ(buffer.BLOCK_SIZE,
        iv0.last - iv0.first);
    EXPECT_EQ(-55, iv0.first[0].channel1);

    EXPECT_EQ(0, buffer.getNumEmpty());
    EXPECT_EQ(buffer.NUM_BLOCKS - 1, buffer.getNumFull());

    buffer.setBlocksEmpty(iv0);
    EXPECT_EQ(1, buffer.getNumEmpty());

    // -- loop through
    for (uint32_t i = 0; i < 100; i++) {
        // with a wrap around we might need two writes
        iv1 = buffer.getEmptyBlocks();
        iv1.first->channel1 = i;
        buffer.setBlocksFull(iv1);

        iv1 = buffer.getEmptyBlocks();
        if (iv1.first != iv1.last) {
            iv1.first->channel1 = i;
            buffer.setBlocksFull(iv1);
        }

        EXPECT_EQ(0, buffer.getNumEmpty());
        EXPECT_EQ(buffer.NUM_BLOCKS, buffer.getNumFull());

        // read three
        for (uint32_t j = 0; j < 3; j++) {
            iv0 = buffer.getFullBlocks();
            buffer.setBlocksEmpty(iv0);
        }
        EXPECT_EQ(3, buffer.getNumEmpty());
    }
}

/** Tests basic functionality for the ringbuffer
 *
 *  Tests
 *
 *  - rcAudio::AudioRingbuffer::setBlocksFull()
 *  - rcAudio::AudioRingbuffer::setBlocksEmpty()
 *
 *  Setting an empty interval or one outside of the block range
 *  should not change anything or even crash.
 */
TEST(AudioRingbufferTest, SetBlocks) {

    rcAudio::AudioRingbuffer buffer;
    auto iv = buffer.getEmptyBlocks();

    // empty inverval
    rcProc::SamplesInterval iv2 = {iv.first, iv.first};

    EXPECT_EQ(0, buffer.getNumFull());
    buffer.setBlocksFull(iv2);
    EXPECT_EQ(0, buffer.getNumFull());

    // outside of the range
    iv2 = {iv.first - 2000, iv.last};
    buffer.setBlocksFull(iv2);
    EXPECT_EQ(0, buffer.getNumFull());

    iv2 = {iv.first + 2000, iv.last};
    buffer.setBlocksFull(iv2);
    EXPECT_EQ(0, buffer.getNumFull());
}

