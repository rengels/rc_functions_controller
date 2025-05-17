/** Tests for proc_engine_reverse.cpp
 *
 *  @file
 */

#include "proc.h"
#include "engine_reverse.h"

#include <gtest/gtest.h>
#include <cmath>
#include <iostream>

using namespace rcSignals;
using namespace rcProc;
using namespace rcEngine;

constexpr float EPSILON = 0.0001f;

/** Unit test for EngineReverse::setDrivingState()
 *
 *  Test if setDrivingState will change the gears
 */
TEST(EngineReverseTest, SetDrivingState) {

    EngineReverse engine;
    engine.fullGears.set({-1.1f, 3.3f, 2.2f, 0.0f});
    engine.start();

    engine.setDrivingState(EngineReverse::DrivingState::FORWARD);
    EXPECT_EQ(2, engine.gears.size());
    EXPECT_EQ(3.3f, engine.gears.get(0));
    EXPECT_EQ(2.2f, engine.gears.get(1));
    EXPECT_EQ(EngineReverse::DrivingState::FORWARD, engine.drivingState);

    engine.setDrivingState(EngineReverse::DrivingState::BACKWARD);
    EXPECT_EQ(1, engine.gears.size());
    EXPECT_EQ(1.1f, engine.gears.get(0));
    EXPECT_EQ(EngineReverse::DrivingState::BACKWARD, engine.drivingState);

    engine.setDrivingState(EngineReverse::DrivingState::STOPPED_BCK);
    EXPECT_EQ(1, engine.gears.size()); // we set gears even in stopped state

}

/** Unit test for EngineReverse::step()
 *
 *  Applies some reverse throttle to see if the gear switching works.
 */
TEST(EngineReverseTest, Reverse) {

    EngineReverse engine;
    engine.idleManager = rcEngine::Idle(100, 100, 0, 0, 100);
    engine.rpmMax = 200u;
    engine.fullGears.set({-1.1f, 3.3f, 2.2f, 0.0f});
    engine.crankingTimeMs = 20;
    engine.massEngine  = 1.0f;
    engine.massVehicle = 2.0f;
    engine.rpmShift = 120;
    engine.gearDecouplingTime = 0;
    engine.gearCouplingTime = 0;
    engine.gearDoubleDeclutchTime = 0;
    engine.resistance = 0.0f;
    engine.airResistance = 0.0f;
    engine.maxPower = 50.0f;
    engine.start();

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 20u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    // cranking
    signals.reset();
    signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;

    EXPECT_EQ(EngineSimple::EngineState::OFF, engine.state);
    EXPECT_EQ(EngineReverse::DrivingState::STOPPED_FWD, engine.drivingState);
    EXPECT_EQ(0, engine.getRPM());

    signals[SignalType::ST_THROTTLE] = -RCSIGNAL_MAX;
    engine.step(info);

    EXPECT_EQ(EngineSimple::EngineState::CRANKING, engine.state);
    EXPECT_EQ(EngineReverse::DrivingState::STOPPED_BCK, engine.drivingState);
    EXPECT_EQ(0, engine.gearCurrent);

    // running
    signals[SignalType::ST_THROTTLE] = -RCSIGNAL_MAX;
    engine.step(info);

    EXPECT_EQ(EngineSimple::EngineState::ON, engine.state);
    EXPECT_EQ(EngineReverse::DrivingState::STOPPED_BCK, engine.drivingState);
    EXPECT_LT(0, engine.getRPM());
    EXPECT_EQ(RCSIGNAL_MAX, signals[SignalType::ST_THROTTLE]);

    // rpm should grow
    float oldRPM = engine.getRPM();
    for (int i = 0; i < 40; i++) {
        signals.reset();
        signals[SignalType::ST_THROTTLE] = -RCSIGNAL_MAX;
        engine.step(info);

        /* for debugging you might want to switch it on
        std::cout << "stat: RPM: " << engine.getRPM() <<
            " gear: " << static_cast<int>(engine.gearCurrent) <<
            " gearNext: " << static_cast<int>(engine.gearNext) <<
            " gear state: " << static_cast<int>(engine.gearState) <<
            " throttle: " << signals[SignalType::ST_THROTTLE] <<
            " speed: " << signals[SignalType::ST_SPEED] <<
            " energyV: " << engine.energyVehicle.get() <<
            std::endl;
        */
    }
    EXPECT_LT(oldRPM, engine.getRPM());
    EXPECT_EQ(EngineReverse::DrivingState::BACKWARD, engine.drivingState);

}
