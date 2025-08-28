/** Tests for proc_click.cpp
 *
 *  @file
 */

#include "proc.h"
#include "proc_click.h"

#include <gtest/gtest.h>

using namespace rcSignals;
using namespace rcProc;


/** Unit test for ProcClick indicator functionality
 *
 */
TEST(ProcClickTest, Simple) {

    ProcClick proc;
    proc.inType = SignalType::ST_AUX1;
    proc.outTypes = {
        SignalType::ST_EX_BUCKET,
        SignalType::ST_EX_DIPPER,
        SignalType::ST_EX_BOOM,
        SignalType::ST_EX_SWING,
        SignalType::ST_NONE
    };

    proc.clickTime = 28;
    proc.start();

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 10u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    // -- no input
    signals.reset();
    proc.step(info);
    EXPECT_EQ(proc.currentClick, 0u);
    EXPECT_EQ(signals[SignalType::ST_EX_BUCKET], RCSIGNAL_INVALID);
    EXPECT_EQ(signals[SignalType::ST_EX_DIPPER], RCSIGNAL_INVALID);
    EXPECT_EQ(signals[SignalType::ST_EX_BOOM], RCSIGNAL_INVALID);
    EXPECT_EQ(signals[SignalType::ST_EX_DIPPER], RCSIGNAL_INVALID);

    // -- go through one click
    signals[SignalType::ST_AUX1] = RCSIGNAL_TRUE;
    proc.step(info);
    EXPECT_EQ(proc.currentClick, 1u);
    EXPECT_EQ(signals[SignalType::ST_EX_BUCKET], RCSIGNAL_INVALID);

    signals[SignalType::ST_AUX1] = RCSIGNAL_NEUTRAL;
    proc.step(info);
    EXPECT_EQ(proc.currentClick, 1u);
    EXPECT_EQ(proc.currentTime, 10u);
    EXPECT_EQ(signals[SignalType::ST_EX_BUCKET], RCSIGNAL_INVALID);

    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_EX_BUCKET], RCSIGNAL_INVALID);

    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_EX_BUCKET], RCSIGNAL_MAX);

    // -- tripple click
    signals[SignalType::ST_AUX1] = RCSIGNAL_TRUE;
    proc.step(info);
    EXPECT_EQ(proc.currentClick, 1u);

    signals[SignalType::ST_AUX1] = RCSIGNAL_NEUTRAL;
    proc.step(info);
    EXPECT_EQ(proc.currentClick, 1u);

    signals[SignalType::ST_AUX1] = RCSIGNAL_TRUE;
    proc.step(info);
    EXPECT_EQ(proc.currentClick, 2u);

    signals[SignalType::ST_AUX1] = RCSIGNAL_NEUTRAL;
    proc.step(info);
    EXPECT_EQ(proc.currentClick, 2u);

    signals[SignalType::ST_AUX1] = RCSIGNAL_TRUE;
    proc.step(info);
    EXPECT_EQ(proc.currentClick, 3u);

    signals[SignalType::ST_AUX1] = RCSIGNAL_NEUTRAL;
    proc.step(info);
    EXPECT_EQ(proc.currentClick, 3u);

    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_EX_BOOM], RCSIGNAL_INVALID);

    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_EX_BOOM], RCSIGNAL_MAX);
}

