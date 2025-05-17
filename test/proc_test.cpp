/** Tests for the effect classes */

#include "proc.h"
#include "proc_indicator.h"
#include "proc_xenon.h"
#include "proc_fade.h"

#include <gtest/gtest.h>

using namespace rcSignals;
using namespace rcProc;

struct TestData {
    RcSignal in1;
    RcSignal in2;
    RcSignal out1;
    RcSignal out2;
};

/** Unit test for ProcIndicator.
 *
 *  Note: the different channels are supposed
 *  to blink in unison.
 */
TEST(ProcTest, Indicator) {

    ProcIndicator effect;
    effect.types = {
      SignalType::ST_CABIN,
      SignalType::ST_ROOF,
      SignalType::ST_NONE,
      SignalType::ST_NONE};
    effect.start();

    const TestData testData[] = {
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL}, //OFF
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL}, //OFF
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL}, //BlinkOn
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL}, //BlinkOn
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL}, //BlinkOn
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL}, //BlinkOff
        {RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL}, //BlinkOff
        {RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX},     //BlinkOn
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_MAX,     RCSIGNAL_MAX},     //BlinkOn
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL}, //BlinkOff
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL}, //BlinkOff
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL}, //BlinkOn
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL}, //BlinkOn
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL}  //OFF
    };

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 330u,  // 330 ms steps
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    signals[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL;
    for (uint32_t i = 0; i < sizeof(testData) / sizeof(testData[0]); i++ ) {
        signals[SignalType::ST_CABIN] = testData[i].in1;
        signals[SignalType::ST_ROOF] = testData[i].in2;

        effect.step(info);
        EXPECT_EQ(testData[i].out1, signals[SignalType::ST_CABIN]) << "in step " << i;
        EXPECT_EQ(testData[i].out2, signals[SignalType::ST_ROOF]) << "in step " << i;
    }
}


/** Unit test for ProcXenon.
 */
TEST(ProcTest, Xenon) {

    ProcXenon effect;
    effect.types = {
      SignalType::ST_CABIN,
      SignalType::ST_ROOF,
      SignalType::ST_NONE,
      SignalType::ST_NONE};
    effect.start();

    Signals signals;
    const TestData testData[] = {
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL}, //OFF
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL},
        {RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX},
        {RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX},
        {RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX - 100U, RCSIGNAL_MAX},
        {RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX - 100U, RCSIGNAL_MAX - 100U},
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL} //OFF
    };

    // 20 ms steps
    rcProc::StepInfo info = {
        .deltaMs = 20u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    signals[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL;
    for (uint32_t i = 0; i < sizeof(testData) / sizeof(testData[0]); i++ ) {
        signals[SignalType::ST_CABIN] = testData[i].in1;
        signals[SignalType::ST_ROOF] = testData[i].in2;

        effect.step(info);
        EXPECT_EQ(testData[i].out1, signals[SignalType::ST_CABIN]) << "in step " << i;
        EXPECT_EQ(testData[i].out2, signals[SignalType::ST_ROOF]) << "in step " << i;
    }
}


/** Unit test for ProcFade.
 *
 * TODO: update, set the effect with different fade in and fade out values
 */
TEST(ProcTest, Fade) {

    ProcFade effect;
    effect.types = {
      SignalType::ST_CABIN,
      SignalType::ST_ROOF,
      SignalType::ST_NONE,
      SignalType::ST_NONE};
    effect.fadeIn = 100;
    effect.fadeOut = 20.;
    effect.start();

    const TestData testData[] = {
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL}, //OFF
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL},
        {RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX,     RCSIGNAL_MAX},
        {RCSIGNAL_MAX,     RCSIGNAL_NEUTRAL, RCSIGNAL_MAX,     RCSIGNAL_MAX - 200},
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_MAX - 200, RCSIGNAL_MAX - 400},
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_MAX - 400, RCSIGNAL_MAX - 600},
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_MAX - 600, RCSIGNAL_MAX - 800},
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_MAX - 800, RCSIGNAL_NEUTRAL},
        {RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL, RCSIGNAL_NEUTRAL},
    };

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 100u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    signals[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL;
    for (uint32_t i = 0; i < sizeof(testData) / sizeof(testData[0]); i++ ) {
        signals[SignalType::ST_CABIN] = testData[i].in1;
        signals[SignalType::ST_ROOF] = testData[i].in2;

        effect.step(info);
        EXPECT_EQ(testData[i].out1, signals[SignalType::ST_CABIN]) << "in step " << i;
        EXPECT_EQ(testData[i].out2, signals[SignalType::ST_ROOF]) << "in step " << i;
    }
}
