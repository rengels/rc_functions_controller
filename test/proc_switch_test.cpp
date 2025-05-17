/** Tests for proc_switch.h */

#include "proc.h"
#include "proc_switch.h"

#include <gtest/gtest.h>
#include <stdio.h>

using namespace rcSignals;
using namespace rcProc;


/** Unit test for ProcSwitch.
 *
 *  A white box test to check if debouncing
 *  the input works.
 *
 *  We just look at posLast and posDebouncedLast.
 */
TEST(ProcSwitchTest, Debouncing) {

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 30u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    rcProc::ProcSwitch proc;
    proc.inType = SignalType::ST_YAW;
    proc.start();

    EXPECT_FALSE(proc.posLast.isValid());
    EXPECT_FALSE(proc.posDebouncedLast.isValid());

    // -- after a step, last is set but not debounced
    signals[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL;
    signals[SignalType::ST_YAW] = RCSIGNAL_NEUTRAL;
    info.deltaMs = proc.TIME_MS_DEBOUNCE + 1;
    proc.step(info);

    EXPECT_EQ(proc.NUM_CHANNELS / 2, proc.posLast.pos);
    EXPECT_TRUE(proc.posLast.isValid());
    EXPECT_FALSE(proc.posDebouncedLast.isValid());

    // -- after another step, last is debounced
    proc.step(info);

    EXPECT_EQ(proc.NUM_CHANNELS / 2, proc.posLast.pos);
    EXPECT_TRUE(proc.posLast.isValid());
    EXPECT_EQ(proc.NUM_CHANNELS / 2, proc.posDebouncedLast.pos);
    EXPECT_TRUE(proc.posDebouncedLast.isValid());

    // -- short differences will not disturb the "debounced" value
    info.deltaMs = proc.TIME_MS_DEBOUNCE - 1;
    signals[SignalType::ST_YAW] = -RCSIGNAL_MAX;
    proc.step(info);
    EXPECT_EQ(0, proc.posLast.pos);
    EXPECT_EQ(proc.NUM_CHANNELS / 2, proc.posDebouncedLast.pos);

    signals[SignalType::ST_YAW] = RCSIGNAL_INVALID;
    proc.step(info);
    EXPECT_EQ(proc.NUM_CHANNELS, proc.posLast.pos);
    EXPECT_EQ(proc.NUM_CHANNELS / 2, proc.posDebouncedLast.pos);

    signals[SignalType::ST_YAW] = RCSIGNAL_NEUTRAL;
    proc.step(info);
    EXPECT_EQ(proc.NUM_CHANNELS / 2, proc.posLast.pos);
    EXPECT_EQ(proc.NUM_CHANNELS / 2, proc.posDebouncedLast.pos);
}

struct PosTestInput {
    rcSignals::SignalType outType;
    rcSignals::RcSignal value;
};

/** Unit test for ProcSwitch.
 *
 *  A black box test for converting input signal to
 *  correct positions.
 *
 *  For five channels the 2000 signal positions should
 *  be split up into 400 blocks.
 *
 *  - -1000 to -600
 *  - -600 to -200
 *  - -200 to 200
 *  - 200 to 600
 *  - 600 to 1000
 */
TEST(ProcSwitchTest, Pos) {

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 30u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    rcProc::ProcSwitch proc;
    proc.inType = SignalType::ST_YAW;
    proc.outTypesMomentary[0] = SignalType::ST_AUX1;
    proc.outTypesMomentary[1] = SignalType::ST_AUX2;
    proc.outTypesMomentary[2] = SignalType::ST_TEMP1;
    proc.outTypesMomentary[3] = SignalType::ST_TEMP2;
    proc.outTypesMomentary[4] = SignalType::ST_VCC;
    proc.start();

    std::array<PosTestInput, 9> inputs = {
        PosTestInput(SignalType::ST_AUX1, -1200),
        PosTestInput(SignalType::ST_AUX1, -1000),
        PosTestInput(SignalType::ST_AUX1, -601),
        PosTestInput(SignalType::ST_AUX2, -599),
        PosTestInput(SignalType::ST_TEMP1, 199),
        PosTestInput(SignalType::ST_TEMP2, 201),
        PosTestInput(SignalType::ST_VCC, 601),
        PosTestInput(SignalType::ST_VCC, 1000),
        PosTestInput(SignalType::ST_VCC, 1200),
    };

    for (const auto& in : inputs) {
        signals[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL;
        signals[SignalType::ST_YAW] = in.value;
        info.deltaMs = proc.TIME_MS_DEBOUNCE + 1;
        proc.step(info);
        proc.step(info);

        EXPECT_EQ(RCSIGNAL_MAX, signals[in.outType]) << " for value " << in.value;
    }
}


/** Unit test for ProcSwitch.
 *
 *  This test checks for the output signals.
 */
TEST(ProcSwitchTest, Output) {

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 30u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    rcProc::ProcSwitch proc;
    proc.inType = SignalType::ST_YAW;
    proc.outTypesMomentary[0] = SignalType::ST_AUX1;
    proc.outTypesShort[0] = SignalType::ST_AUX2;
    proc.outTypesLong[0] = SignalType::ST_TEMP1;
    proc.start();

    EXPECT_FALSE(proc.posLast.isValid());
    EXPECT_FALSE(proc.posDebouncedLast.isValid());

    // -- first step. Not debounced, all outputs NEUTRAL
    signals[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL;
    signals[SignalType::ST_YAW] = -RCSIGNAL_MAX;
    info.deltaMs = proc.TIME_MS_DEBOUNCE + 1;
    proc.step(info);

    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_AUX1]);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_AUX2]);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_TEMP1]);

    // -- Debounced, Momentary high
    signals[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL;
    signals[SignalType::ST_YAW] = -RCSIGNAL_MAX;
    info.deltaMs = proc.TIME_MS_DEBOUNCE + 1;
    proc.step(info);

    EXPECT_EQ(RCSIGNAL_MAX, signals[SignalType::ST_AUX1]);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_AUX2]);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_TEMP1]);

    // -- toggle short
    signals[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL;
    signals[SignalType::ST_YAW] = -RCSIGNAL_MAX;
    info.deltaMs = proc.TIME_MS_TOGGLE + 1;
    proc.step(info);

    signals[SignalType::ST_YAW] = RCSIGNAL_NEUTRAL;
    proc.step(info);
    proc.step(info);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_AUX1]);
    EXPECT_EQ(RCSIGNAL_MAX, signals[SignalType::ST_AUX2]);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_TEMP1]);

    // -- toggle long
    signals[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL;
    signals[SignalType::ST_YAW] = -RCSIGNAL_MAX;
    info.deltaMs = proc.TIME_MS_TOGGLE_LONG + 1;
    proc.step(info);
    proc.step(info);

    signals[SignalType::ST_YAW] = RCSIGNAL_NEUTRAL;
    proc.step(info);
    proc.step(info);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_AUX1]);
    EXPECT_EQ(RCSIGNAL_MAX, signals[SignalType::ST_AUX2]);
    EXPECT_EQ(RCSIGNAL_MAX, signals[SignalType::ST_TEMP1]);
}

