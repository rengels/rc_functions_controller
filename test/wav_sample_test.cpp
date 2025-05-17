/** Tests for the wav_sample.cpp */

#include "wav_sample.h"
#include <gtest/gtest.h>

extern const uint8_t _binary_dummy_wav_start[];
extern const uint8_t _binary_dummy_wav_end[];

/** Unit test for WavSample
 *
 *  This test is using a real wav-file as input.
 */
TEST(WavSampleTest, WavSampleWav) {

    std::span<const uint8_t> sp(
        _binary_dummy_wav_start,
        _binary_dummy_wav_end - _binary_dummy_wav_start);

    auto wavSamples = getWavSamples(sp);

    EXPECT_EQ(_binary_dummy_wav_start + 44,
              &(*wavSamples.begin()));

    EXPECT_EQ(2, wavSamples.size());
}

/** Unit test for WavSample
 *
 *  This test is using a raw sample
 */
TEST(WavSampleTest, WavSampleRaw) {

    const uint8_t raw_samples[6] = {1, 2, 3, 4, 5, 6};

    auto wavSamples = getWavSamples(raw_samples);

    EXPECT_EQ(raw_samples,
              &(*wavSamples.begin()));
    EXPECT_EQ(6, wavSamples.size());
}

