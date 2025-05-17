/** Tests for proc_auto.cpp
 *
 *  @file
 */

#include "proc.h"
#include "proc_auto.h"

#include <gtest/gtest.h>

using namespace rcSignals;
using namespace rcProc;


/** Unit test for ProcAuto indicator functionality
 *
 */
TEST(ProcTest, AutoIndicator) {

    ProcAuto proc;
    proc.start();

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 0u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    // -- with invalid inputs stuff should stay invalid
    signals.reset();
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_LEFT], RCSIGNAL_INVALID);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_RIGHT], RCSIGNAL_INVALID);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_LEFT], RCSIGNAL_INVALID);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_RIGHT], RCSIGNAL_INVALID);

    // -- with valid inputs stuff should stay neutral
    signals.reset();
    signals[SignalType::ST_YAW] = RCSIGNAL_NEUTRAL;
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_LEFT], RCSIGNAL_NEUTRAL);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_RIGHT], RCSIGNAL_NEUTRAL);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_LEFT], RCSIGNAL_NEUTRAL);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_RIGHT], RCSIGNAL_NEUTRAL);

    // -- steering should switch the indicators on
    signals.reset();
    signals[SignalType::ST_YAW] = RCSIGNAL_MAX;
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_LEFT], RCSIGNAL_MAX);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_RIGHT], RCSIGNAL_NEUTRAL);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_LEFT], RCSIGNAL_MAX);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_RIGHT], RCSIGNAL_NEUTRAL);

    signals.reset();
    signals[SignalType::ST_YAW] = -RCSIGNAL_MAX;
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_LEFT], RCSIGNAL_NEUTRAL);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_RIGHT], RCSIGNAL_MAX);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_LEFT], RCSIGNAL_NEUTRAL);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_RIGHT], RCSIGNAL_MAX);

    // -- hazard lights
    signals.reset();
    signals[SignalType::ST_YAW] = RCSIGNAL_NEUTRAL;
    signals[SignalType::ST_LI_HAZARD] = RCSIGNAL_MAX;
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_LEFT], RCSIGNAL_NEUTRAL);
    EXPECT_EQ(signals[SignalType::ST_LI_INDICATOR_RIGHT], RCSIGNAL_NEUTRAL);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_LEFT], RCSIGNAL_MAX);
    EXPECT_EQ(signals[SignalType::ST_INDICATOR_RIGHT], RCSIGNAL_MAX);
}

