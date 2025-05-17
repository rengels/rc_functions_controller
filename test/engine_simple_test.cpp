/** Tests for engine_simple.cpp
 *
 *  @file
 */

#include "proc.h"
#include "engine_simple.h"

#include <gtest/gtest.h>

using namespace rcSignals;
using namespace rcProc;
using namespace rcEngine;

constexpr float EPSILON = 0.0001f;


/** Unit test for rcProc::Energy functions.
 *
 *  Tests
 *
 *  - Energy::add()
 *  - Energy::set()
 *  - Energy::get()
 *  - Energy::energyFromSpeed()
 *  - Energy::speed()
 */
TEST(EnergyTest, Energy) {

    rcEngine::Energy e;

    e.set(0.0f);

    EXPECT_NEAR(0.0f, e.get(), EPSILON);

    e.add(1.0f);
    EXPECT_NEAR(1.0f, e.get(), EPSILON);

    // energy does not get negative
    e.add(-2.0f);
    EXPECT_NEAR(0.0f, e.get(), EPSILON);

    EXPECT_NEAR(1.0f, e.energyFromSpeed(2.0f, 0.5f), EPSILON);

    e.set(1.0f);
    EXPECT_NEAR(2.0f, e.speed(0.5f), EPSILON);
}


/** Unit test for EngineSimple energy functions.
 *
 *  Tests
 *
 *  - EngineSimple::getRPM()
 *  - EngineSimple::setRPM()
 */
TEST(EngineSimpleTest, Energy) {

    EngineSimple engine;
    engine.start();

    // set RPM vs get RPM
    engine.setRPM(1000.0f);
    EXPECT_NEAR(1000.0f, engine.getRPM(), EPSILON);
}


/** Unit test for EngineSimple RPM/throttle
 *
 *  Full throttle should bring RPM to about max RPM.
 *  No throttle should brint it back down.
 */
TEST(EngineSimpleTest, RPM) {

    EngineSimple engine;
    engine.engineType = EngineSimple::EngineType::PETROL;
    engine.crankingTimeMs = 20u;
    engine.massEngine = 2000.0f;
    engine.maxPower = 20000.0f;
    engine.rpmMax = 400.0f;
    engine.idleManager = rcEngine::Idle(100, 100, 0, 0, 20);
    engine.start();

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 0u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    // -- throttle should switch the motor on
    // cranking
    signals.reset();
    signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
    info.deltaMs = 100U;

    engine.step(info);

    // running
    signals.reset();
    signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_TRUE;
    info.deltaMs = 21U;

    engine.step(info);

    EXPECT_NEAR(100, signals[SignalType::ST_RPM], 10); // idle RPM

    // -- bring throttle up
    for (int i = 0; i < 10; i++) {
        signals.reset();
        signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
        signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
        info.deltaMs = 300U;

        engine.step(info);
    }

    EXPECT_LT(300, signals[SignalType::ST_RPM]);

    // -- bring throttle down
    for (int i = 0; i < 40; i++) {
        signals.reset();
        signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
        signals[SignalType::ST_THROTTLE] = RCSIGNAL_NEUTRAL;
        info.deltaMs = 300U;
        engine.step(info);
    }

    EXPECT_GT(200, signals[SignalType::ST_RPM]);
}

/** Unit test for EngineSimple functionality
 *
 *  Unit test for rcProc::EngineSimple::stepEngine()
 */
TEST(EngineSimpleTest, StepEngine) {

    EngineSimple engine;
    engine.crankingTimeMs = 50;
    engine.start();

    engine.stepEngine(100u, RCSIGNAL_INVALID);
    EXPECT_EQ(EngineSimple::EngineState::OFF, engine.state);

    // - cranking
    engine.stepEngine(100u, RCSIGNAL_MAX);
    EXPECT_EQ(EngineSimple::EngineState::CRANKING, engine.state);

    // - on
    engine.stepEngine(100u, RCSIGNAL_MAX);
    EXPECT_EQ(EngineSimple::EngineState::ON, engine.state);

    // - off
    engine.stepEngine(100u, RCSIGNAL_NEUTRAL);
    EXPECT_EQ(EngineSimple::EngineState::OFF, engine.state);
}

/** Unit test for EngineSimple functionality
 *
 *  Unit test for rcProc::EngineSimple::step()
 */
TEST(EngineSimpleTest, Step) {

    EngineSimple engine;
    engine.start();

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 0u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    // -- with invalid inputs stuff should stay idle
    signals.reset();

    engine.step(info);

    EXPECT_EQ(signals[SignalType::ST_IGNITION], RCSIGNAL_NEUTRAL);
    EXPECT_EQ(signals[SignalType::ST_RPM], 0);
}

