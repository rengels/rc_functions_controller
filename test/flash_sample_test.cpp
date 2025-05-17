/** Tests for the flash_sample.h */

#include "flash_sample.h"
#include <gtest/gtest.h>

using namespace FlashSample;

/** Unit test for FlashSingleton
 *
 *  tests
 *  - FlashSingleton::flush()
 *  - FlashSingleton::setData()
 *  - FlashSingleton::data()
 *  - FlashSingleton::getIndex()
 *  - FlashSingleton::reset()
 */
TEST(FlashSampleTest, FlashSingleton) {

    auto& flash = FlashSingleton::getInstance();

    const uint8_t* dt0 = static_cast<const uint8_t*>(flash.data(0));
    const uint8_t* dt1 = static_cast<const uint8_t*>(flash.data(1));
    const uint8_t* dt2 = static_cast<const uint8_t*>(flash.data(2));

    EXPECT_EQ(dt1, dt0 + SPI_FLASH_SEC_SIZE);
    EXPECT_EQ(dt2, dt1 + SPI_FLASH_SEC_SIZE);

    EXPECT_EQ(0, flash.getIndex(dt0));
    EXPECT_EQ(1, flash.getIndex(dt1));
    EXPECT_EQ(1, flash.getIndex(dt1 + 100));

    const uint8_t rawData[] = {1, 2, 3, 4, 5, 6};
    std::span<const uint8_t> sp(rawData);

    flash.setData(1, 2, sp);
    flash.flush();
    EXPECT_EQ(1, dt1[2]);
    EXPECT_EQ(6, dt1[2 + 5]);

    flash.setData(1, 0, sp);
    // should not immediately set the data
    EXPECT_EQ(1, dt1[2]);
    flash.flush();
    EXPECT_EQ(3, dt1[2]);
    EXPECT_EQ(1, dt1[0]);
    EXPECT_EQ(6, dt1[0 + 5]);

    flash.reset(1);
    EXPECT_EQ(0x00, dt1[0]);
}


/** Unit test for SampleBlock
 *
 *  tests
 *  - SampleBlock::first()
 *  - SampleBlock::next()
 *  - SampleBlock::data()
 *  - SampleBlock::addBlock()
 *  - SampleBlock::setData()
 *  - SampleBlock::maxSize()
 *  - SampleBlock::getFile()
 */
TEST(FlashSampleTest, SampleBlock) {

    auto& flash = FlashSingleton::getInstance();
    flash.reset(0);
    flash.reset(1);
    flash.reset(3);
    flash.reset(4);

    // -- first, addBlock
    ASSERT_EQ(nullptr, SampleBlock::first());
    auto block1 = SampleBlock::addBlock(rcSamples::AudioId({'a', 'b', 'c'}), 100);
    flash.flush();
    ASSERT_NE(nullptr, block1);
    EXPECT_EQ(block1, SampleBlock::first());

    // block to big
    auto block5 = SampleBlock::addBlock(rcSamples::AudioId({'x', 'y', 'z'}), 10000000);
    flash.flush();
    ASSERT_EQ(nullptr, block5);

    // -- next, data
    EXPECT_EQ(nullptr, block1->next());
    EXPECT_LT(flash.data(0), block1->data());

    // -- getFile, maxSize
    EXPECT_EQ(SPI_FLASH_SEC_SIZE - sizeof(SampleBlock), block1->maxSize());
    auto file = block1->getFile();
    EXPECT_EQ(rcSamples::AudioId({'a', 'b', 'c'}), file.id);
    EXPECT_EQ(100, file.content.size());

    // -- setData
    auto block2 = SampleBlock::addBlock(rcSamples::AudioId({'A', 'B', 'C'}), 5500);
    ASSERT_NE(nullptr, block2);
    EXPECT_EQ(block2, block1->next()); // block can be used immediately
    EXPECT_EQ(nullptr, block2->next());
    EXPECT_EQ(5500, block2->size);

    const uint8_t rawData[] = {1, 2, 3, 4, 5, 6};
    std::span<const uint8_t> sp(rawData);
    ASSERT_EQ(6, sp.size());
    block2->setData(2, sp);

    ASSERT_EQ(6, sp.size());
    block2->setData(5002, sp);
    flash.flush();
    EXPECT_EQ(1, block2->data()[2]);
    EXPECT_EQ(1, block2->data()[5002]);

    // -- getFile
    file = block2->getFile();
    EXPECT_EQ(rcSamples::AudioId({'A', 'B', 'C'}), file.id);
    EXPECT_EQ(block2->data(), file.content.data());
    EXPECT_EQ(5500, file.content.size());
    EXPECT_EQ(1, file.content.data()[2]);

    // -- reset
    block2->reset();
    flash.flush();
    EXPECT_EQ(nullptr, block1->next());
}

/** Unit test for SampleStorage
 *
 *  tests
 *  - SampleBlock::reset()
 *  - SampleBlock::getFiles()
 *  - SampleBlock::addId()
 *  - SampleBlock::setData()
 */
TEST(FlashSampleTest, SampleStorage) {

    auto& flash = FlashSingleton::getInstance();
    flash.reset(0);
    flash.reset(1);
    flash.reset(3);
    flash.reset(4);

    SampleBlock::addBlock(rcSamples::AudioId({'a', 'b', 'c'}), 100);
    SampleBlock::addBlock(rcSamples::AudioId({'A', 'B', 'C'}), 200);

    SampleStorage storage = SampleStorage();

    // -- getFiles
    auto files = storage.getFiles();
    EXPECT_EQ(2, files.size());

    // -- addId
    ASSERT_TRUE(storage.addId(rcSamples::AudioId({'C', 'B', 'A'}), 300));

    files = storage.getFiles();
    ASSERT_EQ(3, files.size());
    EXPECT_EQ(rcSamples::AudioId({'C', 'B', 'A'}), files[2].id);
    ASSERT_EQ(100, files[0].content.size());
    ASSERT_EQ(200, files[1].content.size());
    ASSERT_EQ(300, files[2].content.size());

    // -- setData
    const uint8_t rawData[] = {1, 2, 3, 4, 5, 6};
    std::span<const uint8_t> sp(rawData);
    storage.setData(rcSamples::AudioId({'C', 'B', 'A'}), 9, sp);
    flash.flush();

    files = storage.getFiles();
    EXPECT_EQ(1, files[2].content.data()[9]);
    EXPECT_EQ(6, files[2].content.data()[9 + 5]);

    // -- sectors used/free
    EXPECT_EQ(3, storage.sectorsUsed());
    EXPECT_EQ(flash.getMaxSectors() - 3, storage.sectorsFree());

    // -- reset
    storage.reset();
    files = storage.getFiles();
    ASSERT_EQ(0, files.size());

    EXPECT_EQ(0, storage.sectorsUsed());
    EXPECT_EQ(flash.getMaxSectors(), storage.sectorsFree());
}

