/** Tests for the Audio classes class */

#include "signals.h"
#include "proc.h"

#include "audio_simple.h"
#include "audio_loop.h"
#include "audio_engine.h"

#include <gtest/gtest.h>

using namespace rcAudio;
using namespace rcSignals;

static uint8_t samples[] = {8, 7, 6, 5, 4, 3, 2, 1, 0, 11, 12, 13, 14, 15, 16, 17};
static SampleData testSample(samples);

// a shorter test sample (has to be at least 10 samples or it will be considered empty)
static uint8_t samples2[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
static SampleData testSample2(samples2);

// an empty test sample
static std::array<const uint8_t, 0> samples3;
static SampleData testSampleEmpty(samples3);


/** Tests AudioSimple class
 *
 */
TEST(AudioTest, AudioSimple) {
    rcProc::AudioSample buffer[16];

    AudioSimple sound(testSample, SignalType::ST_THROTTLE);
    sound.start();

    rcSignals::Signals signals;
    signals.reset();

    rcProc::StepInfo info = {
        .deltaMs = 20U,
        .signals = &signals,
        .intervals = {
            rcProc::SamplesInterval{.first = buffer, .last = buffer + 10},
            rcProc::SamplesInterval{.first = buffer, .last = buffer}
            }
    };

    // -- call main without a trigger
    for (uint8_t i = 0; i < 16; i++) {
        buffer[i].channel1 = 0;
    }
    sound.step(info);
    for (uint8_t i = 0; i < 16; i++) {
        EXPECT_EQ(0, buffer[i].channel1);
    }

    // -- call main with trigger
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
    for (uint8_t i = 0; i < 16; i++) {
        buffer[i].channel1 = 0;
    }
    sound.step(info);
    EXPECT_EQ(8 - 128, buffer[0].channel1);
    EXPECT_EQ(7 - 128, buffer[1].channel1);
    EXPECT_EQ(11 - 128,  buffer[9].channel1);
    EXPECT_EQ(0,  buffer[10].channel1);

    // -- sound should play until the end and then stop
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
    for (uint8_t i = 0; i < 16; i++) {
        buffer[i].channel1 = 0;
    }
    sound.step(info);
    EXPECT_EQ(12 - 128, buffer[0].channel1);
    EXPECT_EQ(13 - 128, buffer[1].channel1);
    EXPECT_EQ(17 - 128, buffer[5].channel1);
    EXPECT_EQ(0, buffer[6].channel1);
    EXPECT_EQ(0, buffer[7].channel1);

    // -- only a re-trigger should activate it again
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_NEUTRAL;
    for (uint8_t i = 0; i < 16; i++) {
        buffer[i].channel1 = 0;
    }
    sound.step(info);
    EXPECT_EQ(0, buffer[0].channel1);
    EXPECT_EQ(0, buffer[7].channel1);

    signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
    sound.step(info);
    EXPECT_EQ(8 - 128, buffer[0].channel1);
    EXPECT_EQ(7 - 128, buffer[1].channel1);
}


/** Tests AudioLoop class
 *
 *  We test two scenarios:
 *  - loop from 1 to 10
 *  - loop from 0 to 16
 */
TEST(AudioTest, AudioLoop) {
    rcProc::AudioSample buffer[32];

    rcSignals::Signals signals;
    signals.reset();

    rcProc::StepInfo info = {
        .deltaMs = 20U,
        .signals = &signals,
        .intervals = {
            rcProc::SamplesInterval{.first = buffer, .last = buffer + 9},
            rcProc::SamplesInterval{.first = buffer + 9, .last = buffer + 32}
            }
    };

    // -- scenario 1 to 10
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
    for (uint8_t i = 0; i < 32; i++) {
        buffer[i].channel1 = 0;
    }

    AudioLoop sound1(testSample, 1, 10, SignalType::ST_THROTTLE);
    sound1.start();

    sound1.step(info);
    EXPECT_EQ(8 - 128,  buffer[0].channel1);
    EXPECT_EQ(7 - 128,  buffer[1].channel1);
    EXPECT_EQ(0 - 128, buffer[8].channel1);
    EXPECT_EQ(11 - 128, buffer[9].channel1);
    EXPECT_EQ(7 - 128,  buffer[10].channel1); // loop back
    EXPECT_EQ(6 - 128,  buffer[11].channel1);
    EXPECT_EQ(5 - 128,  buffer[12].channel1);
    EXPECT_EQ(11 - 128,  buffer[18].channel1);
    EXPECT_EQ(7 - 128, buffer[19].channel1); // loop back

    // -- scenario 0 to 16
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
    for (uint8_t i = 0; i < 32; i++) {
        buffer[i].channel1 = 0;
    }

    AudioLoop sound2(testSample, 0, 16, SignalType::ST_THROTTLE);
    sound2.start();

    signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
    for (uint8_t i = 0; i < 32; i++) {
        buffer[i].channel1 = 0;
    }
    sound2.step(info);
    EXPECT_EQ(8 - 128,  buffer[0].channel1);
    EXPECT_EQ(7 - 128,  buffer[1].channel1);
    EXPECT_EQ(11 - 128, buffer[9].channel1);
    EXPECT_EQ(17 - 128, buffer[15].channel1);
    EXPECT_EQ(8 - 128,  buffer[16].channel1); // loop back
    EXPECT_EQ(7 - 128,  buffer[17].channel1);
    EXPECT_EQ(17 - 128, buffer[31].channel1);
}

/** Tests AudioEngine::getVolumes()
 *
 *  Smoke test with some error cases
 *  - no valid samples
 *  - only one sample (or valid sample)
 *  - four quadrants
 */
TEST(AudioEngineTest, getVolumes) {
    // -- scenario 1: no valid sample
    // actually the results shouldn't matter.
    // it should just not crash
    AudioEngine audio({testSampleEmpty, testSampleEmpty});
    audio.start();

    auto volumes = audio.getVolumes(0.0f, 0);

    // -- scenario 2: two samples
    audio = AudioEngine({testSample, testSample2}, {100, 1000});
    audio.start();

    volumes = audio.getVolumes(1.0f, 1);
    EXPECT_NEAR(1.0f, volumes[0], 0.1f);
    EXPECT_NEAR(0.0f, volumes[1], 0.1f);
    EXPECT_NEAR(0.0f, volumes[2], 0.1f);
    EXPECT_NEAR(0.0f, volumes[3], 0.1f);

    // -- scenario 3: four samples, all same volume
    audio = AudioEngine({testSample, testSample, testSample2, testSample2},
                        {0, 1000, 0, 1000});
    audio.start();

    float midRPM = ((audio.rpms[0] + audio.rpms[2]) / 2.0f);

    volumes = audio.getVolumes(midRPM, 500);
    EXPECT_NEAR(0.25f, volumes[0], 0.1f);
    EXPECT_NEAR(0.25f, volumes[1], 0.1f);
    EXPECT_NEAR(0.25f, volumes[2], 0.1f);
    EXPECT_NEAR(0.25f, volumes[3], 0.1f);
}

