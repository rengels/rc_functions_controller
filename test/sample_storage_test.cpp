/** Tests for the SampleStorageSingleton class */

#include "sample_storage_singleton.h"
#include "simple_byte_stream.h"
#include <gtest/gtest.h>

extern const uint8_t _binary_whistle_wav_start[];
extern const uint8_t _binary_whistle_wav_end[];


/** Test:
 *
 *  - SampleStorageSingleton::getSampleFile()
 *  - SampleStorageSingleton::getSampleData()
 *  - SampleStorageSingleton::getAudioId()
 *
 *  all for static files
 */
TEST(SSTest, StaticSamples) {
    auto& ss = SampleStorageSingleton::getInstance();

    // that should be our steam whistle
    auto& file = ss.getSampleFile(rcSamples::AudioId({'T', 'S', 'W'}));
    EXPECT_EQ(_binary_whistle_wav_start, &(*file.content.begin()));
    EXPECT_EQ(_binary_whistle_wav_end - _binary_whistle_wav_start, file.content.size());
    EXPECT_TRUE(file.content.size() < 50000);
    EXPECT_TRUE(file.content.size() > 1000);
    EXPECT_EQ(rcSamples::AudioId({'T', 'S', 'W'}), file.id);

    auto& data = ss.getSampleData(rcSamples::AudioId({'T', 'S', 'W'}));
    EXPECT_TRUE(data.size() > 1000);

    auto& id = ss.getAudioId(data);
    EXPECT_EQ(file.id, id);

    // now get an unknown file
    auto& file2 = ss.getSampleFile(rcSamples::AudioId({'X', 'Y', 'Z'}));
    EXPECT_TRUE(file2.content.size() > 1000); // that should be a valid file
}


/** Test:
 *
 *  - SampleStorageSingleton::executeCommand()
 *
 *  for dynamic files
 */
TEST(SSTest, Command) {
    auto& ss = SampleStorageSingleton::getInstance();

    rcAudio::SampleData sampleSpan(_binary_whistle_wav_start,
       static_cast<size_t>(_binary_whistle_wav_end - _binary_whistle_wav_start));

    rcSamples::AudioId id({'e', 'f', 'g'});  // our test ID

    const uint8_t bufRemove[] = {'R', 'A', 1,
        0x00u, 0x65u, 0x66u, 0x67u};  // efg
    std::span<const uint8_t> spRemove(bufRemove);

    const uint8_t bufAdd[] = {'R', 'A', 1,
        0x01u, 0x65u, 0x66u, 0x67u, 0, 0, 0, 100};  // efg
    std::span<const uint8_t> spAdd(bufAdd);

    const uint8_t bufAddData[] = {'R', 'A', 1,
        0x02u, 0x65u, 0x66u, 0x67u,
        0, 0, 0, 1,  // offset
        0, 0, 0, 10,  // size
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // data
    std::span<const uint8_t> spAddData(bufAddData);

    // -- add
    SimpleInStream in1(spAdd);
    ss.executeCommand(in1);
    auto file = ss.getSampleFile(id);
    EXPECT_EQ(100u, file.content.size());
    EXPECT_EQ(id, file.id);

    // -- add Data
    SimpleInStream in2(spAddData);
    ss.executeCommand(in2);
    auto file2 = ss.getSampleFile(id); // does a flush for flash
    EXPECT_EQ(1u, file2.content.begin()[1]);
    EXPECT_EQ(2u, file2.content.begin()[2]);

    // -- remove
    SimpleInStream in3(spRemove);
    ss.executeCommand(in3);
    auto file3 = ss.getSampleFile(id);
    EXPECT_FALSE(id == file3.id);
}


/** Test:
 *
 *  - SampleStorageSingleton::getSampleData()
 *  - SampleStorageSingleton::getAudioId()
 *
 *  for static files
 */
TEST(SSTest, SampleData) {
    auto& ss = SampleStorageSingleton::getInstance();

    auto& data = ss.getSampleData(rcSamples::AudioId({'T', 'S', 'W'}));

    // check start of the sample
    EXPECT_EQ(0x73u, data[0]);
    EXPECT_EQ(0x6cu, data[1]);
    EXPECT_TRUE(data.size() < 50000);
    EXPECT_TRUE(data.size() > 1000);

    auto& id = ss.getAudioId(data);
    EXPECT_EQ(rcSamples::AudioId({'T', 'S', 'W'}), id);
}

