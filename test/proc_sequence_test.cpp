/** Tests for proc_sequence.cpp
 *
 *  @file
 */

#include "proc.h"
#include "proc_sequence.h"

#include <gtest/gtest.h>

using namespace rcSignals;
using namespace rcProc;


/** Unit test for ProcSequence indicator functionality
 *
 */
TEST(ProcSequenceTest, Sequence) {

    ProcSequence proc;
    proc.onOffTimes = {12, 20, 30, 40};
    proc.sequenceDurationMs = 80;
    proc.inputType = SignalType::ST_AUX1;
    proc.outputType = SignalType::ST_AUX2;
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

    // -- with invalid inputs stuff should stay invalid
    signals.reset();
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_INVALID);

    // -- with neutral inputs stuff should stay neutral
    signals.reset();
    signals[SignalType::ST_AUX1] = RCSIGNAL_NEUTRAL;
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_NEUTRAL);
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_NEUTRAL);
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_NEUTRAL);

    // -- test sequence
    signals.reset();
    signals[SignalType::ST_AUX1] = RCSIGNAL_MAX;
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_NEUTRAL); // 10
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_NEUTRAL); // 20
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_MAX); // 30
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_MAX); // 40
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_NEUTRAL); // 50
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_NEUTRAL); // 60
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_NEUTRAL); // 70
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_MAX); // 80
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_MAX); // 90
    proc.step(info);
    EXPECT_EQ(signals[SignalType::ST_AUX2], RCSIGNAL_NEUTRAL); // 100

}

