/** Tests for proc_engine_gear.cpp
 *
 *  @file
 */

#include "proc.h"
#include "engine_gear.h"

#include <gtest/gtest.h>
#include <cmath>
#include <signals.h>
#include <iostream>


using namespace rcSignals;
using namespace rcProc;
using namespace rcEngine;

/** Small value for EXPECT_NEAR */
constexpr float EPSILON = 0.0001f;

/** Unit test for EngineGear::updateGearList()
 *
 *  Also tests numGears and size()
 */
TEST(GearCollectionTest, UpdateGearList) {
    GearCollection gears;

    // case 1: negative and positives
    gears.gearRatios = {1.0f, 0.0f, 2.0f, -2.0f, -3.0f, 0.0};
    gears.updateGearList();

    EXPECT_NEAR(-3.0f, gears.gearRatios[0], EPSILON);
    EXPECT_NEAR(-2.0f, gears.gearRatios[1], EPSILON);
    EXPECT_NEAR(2.0f, gears.gearRatios[2], EPSILON);
    EXPECT_NEAR(1.0f, gears.gearRatios[3], EPSILON);
    EXPECT_NEAR(0.0f, gears.gearRatios[4], EPSILON);

    EXPECT_NEAR(-2.0f, gears.get(1), EPSILON);
    EXPECT_EQ(4, gears.numGears);
    EXPECT_EQ(4, gears.size());

    // case 2: only positives
    gears.gearRatios = {1.0f, 0.0f, 2.0f, 0.0};
    gears.updateGearList();

    EXPECT_NEAR(2.0f, gears.gearRatios[0], EPSILON);
    EXPECT_NEAR(1.0f, gears.gearRatios[1], EPSILON);
    EXPECT_NEAR(0.0f, gears.gearRatios[2], EPSILON);

    EXPECT_EQ(2, gears.numGears);
}

/** Unit test for EngineGear::forwardGears() and ProgEngineGear::rearGears()
 */
TEST(GearCollectionTest, Forward) {
    GearCollection gears;
    gears.set({1.0f, 2.0f, 3.0f, -2.0f, -3.0f, 0.0});

    auto gearsF = gears.forwardGears();
    EXPECT_EQ(3, gearsF.size());
    EXPECT_EQ(3.0f, gearsF.get(0));

    auto gearsR = gears.rearGears();
    EXPECT_EQ(2, gearsR.size());
    EXPECT_EQ(3.0f, gearsR.get(0)); // rear gears get inverted
}


/** Unit test for EngineGear::getRotationRatio() */
TEST(EngineGearTest, RotationRatio) {

    EngineGear engine;
    engine.wheelDiameter = 1 / M_PI;
    engine.gears.set({2.0f, 1.0f, 0.0f});
    engine.start();

    EXPECT_NEAR(1.0, engine.getRotationRatio(0), EPSILON);
    EXPECT_TRUE(engine.getRotationRatio(1) > 1.0);
    float rr1 = engine.getRotationRatio(1);

    EXPECT_NEAR(rr1 / 2, engine.getRotationRatio(2), EPSILON);
}


/** Unit test for EngineGear::vehicleEnergyFactor() */
TEST(EngineGearTest, VehicleEnergyFactor) {

    EngineGear engine;
    engine.wheelDiameter = 1 / M_PI;
    engine.gears.set({2.0f, 1.0f, 0.0f});
    engine.start();

    // -- vehicleEnergyFactor
    engine.massEngine = 1.0f;
    engine.massVehicle = 1.0f;
    EXPECT_NEAR(1.0, engine.vehicleEnergyFactor(0), EPSILON);
    float ef1 = engine.vehicleEnergyFactor(0);

    // twice the mass means twice the energy factor
    engine.massVehicle = 2.0f;
    EXPECT_NEAR(ef1 * 2.0f, engine.vehicleEnergyFactor(0), EPSILON);
    engine.massVehicle = 1.0f;

    // higher gear means higher energy factor
    float ef2 = engine.vehicleEnergyFactor(1);
    EXPECT_NEAR(ef2 * 4.0, engine.vehicleEnergyFactor(2), EPSILON);
}


/** Unit test for EngineGear::distributeEnergy() */
TEST(EngineGearTest, DistributeEnergy) {

    EngineGear engine;

    engine.wheelDiameter = 1 / M_PI;
    engine.gears.set({1.0f, 0.0f});
    engine.massEngine = 1.0f;
    engine.massVehicle = 2.0f;
    engine.start();
    engine.gearCurrent = 1;
    EXPECT_NEAR(2.0, engine.vehicleEnergyFactor(1), EPSILON);

    // -- Completely balance it
    engine.energyEngine.set(300);
    engine.energyVehicle.set(0);
    EXPECT_FALSE(engine.isEnergyBalanced());

    engine.distributeEnergy(0.0f, 1000.0f, 9999.9f);
    EXPECT_NEAR(100, engine.energyEngine.get(), EPSILON);
    EXPECT_NEAR(200, engine.energyVehicle.get(), EPSILON);
    EXPECT_TRUE(engine.isEnergyBalanced());

    // -- keep min RPM-energy
    engine.energyEngine.set(300);
    engine.energyVehicle.set(0);
    EXPECT_FALSE(engine.isEnergyBalanced());

    engine.distributeEnergy(300.0f, 1000.0f, 9999.9f);
    EXPECT_NEAR(300, engine.energyEngine.get(), EPSILON);
    EXPECT_NEAR(0, engine.energyVehicle.get(), EPSILON);
    EXPECT_FALSE(engine.isEnergyBalanced());

    // -- max energy transfer
    // TODO
}

/** Unit test for EngineGear::rpmForGear()
 */
TEST(EngineGearTest, RpmForGear) {

    EngineGear engine;
    engine.gears.set({2.0f, 1.0f, 0.0f});
    engine.wheelDiameter = 1 / M_PI;
    engine.massEngine = 1.0f;
    engine.massVehicle = 2.0f;
    engine.start();

    engine.gearCurrent = 0;
    engine.setRPM(300);

    EXPECT_NEAR(300, engine.getRPM(), 10.0f);
    EXPECT_LT(0, engine.energyEngine.get());
    EXPECT_NEAR(0, engine.energyVehicle.get(), 1.0f);

    EXPECT_NEAR(300, engine.rpmForGear(0), 10.0f);
    EXPECT_NEAR(250, engine.rpmForGear(1), 10.0f);
    EXPECT_NEAR(170, engine.rpmForGear(2), 10.0f);
}

/** Unit test for EngineGear::chooseGear()
 */
TEST(EngineGearTest, ChooseGear) {

    EngineGear engine;

    engine.gears.set({2.0f, 1.2f, 1.0f, 0.0f});
    engine.idleManager = rcEngine::Idle(100, 100, 0, 0, 100);
    engine.rpmShift = 200.0f;
    engine.massEngine = 10.0f;
    engine.massVehicle = 30.0f;
    engine.start();

    // want to accelerate, upshift
    engine.gearCurrent = 0;
    engine.setRPM(500.0f);

    EXPECT_EQ(1, engine.chooseGear(true));

    // no throttle, upshift as long as we are over idle RPM
    engine.gearCurrent = 1;
    EXPECT_EQ(2, engine.chooseGear(false));

    engine.gearCurrent = 2;
    EXPECT_EQ(2, engine.chooseGear(true));

    // throttle, downshift to keep over rpmShift
    engine.setRPM(100.0f);
    engine.gearCurrent = 2;
    EXPECT_EQ(1, engine.chooseGear(true));

    // no, upshift as long as we are over idle RPM
    engine.setRPM(500.0f);
    engine.gearCurrent = 1;
    EXPECT_EQ(2, engine.chooseGear(false));
}

/** Unit test for EngineGear::selectGear()
 */
TEST(EngineGearTest, SelectGear) {

    EngineGear engine;
    engine.gearCouplingTime = 2u;
    engine.gearDecouplingTime = 3u;
    engine.gearDoubleDeclutchTime = 4u;
    engine.start();
    engine.gearCurrent = 2u;
    engine.gearNext = 2u;

    // -- COUPLED
    engine.selectGear(10);
    EXPECT_EQ(2, engine.gearCurrent);
    EXPECT_EQ(0, engine.gearStepTime);
    EXPECT_EQ(EngineGear::GearState::COUPLED, engine.gearState);

    // -- stay COUPLED
    engine.selectGear(10);
    EXPECT_EQ(2, engine.gearCurrent);
    EXPECT_EQ(0, engine.gearStepTime);
    EXPECT_EQ(EngineGear::GearState::COUPLED, engine.gearState);

    // -- DECOUPLING
    engine.gearNext = 1u;
    engine.selectGear(10);
    EXPECT_EQ(2, engine.gearCurrent);
    EXPECT_EQ(0, engine.gearStepTime);
    EXPECT_EQ(EngineGear::GearState::DECOUPLING, engine.gearState);

    // -- DECOUPLED
    engine.selectGear(10);
    EXPECT_EQ(2, engine.gearCurrent);
    EXPECT_EQ(7, engine.gearStepTime);
    EXPECT_EQ(EngineGear::GearState::DECOUPLED, engine.gearState);

    // -- COUPLING
    engine.selectGear(10);
    EXPECT_EQ(1, engine.gearCurrent);
    EXPECT_EQ(13, engine.gearStepTime);
    EXPECT_EQ(EngineGear::GearState::COUPLING, engine.gearState);

    // -- COUPLED
    engine.selectGear(10);
    EXPECT_EQ(1, engine.gearCurrent);
    EXPECT_EQ(21, engine.gearStepTime);
    EXPECT_EQ(EngineGear::GearState::COUPLED, engine.gearState);
}

/** Unit test for EngineGear for auto ignition
 *
 */
TEST(EngineGearTest, Ignition) {

    EngineGear engine;
    engine.engineType = EngineSimple::EngineType::PETROL;
    engine.massEngine = 2000.0f;
    engine.maxPower = 2000.0f;  // keep it low or it might spool up to fast
    engine.crankingTimeMs = 1000u;
    engine.offTimeMs = 1000u;
    engine.rpmMax = 400u;
    engine.idleManager = rcEngine::Idle(100, 100, 0, 0, 100);
    engine.start();

    Signals signals;
    rcProc::StepInfo info = {
        .deltaMs = 10u,
        .signals = &signals,
        .intervals = {
            SamplesInterval{.first = nullptr, .last = nullptr},
            SamplesInterval{.first = nullptr, .last = nullptr}
        }
    };

    // -- with throttle 0 the ignition should be off and stay off
    signals.reset();
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_NEUTRAL;

    engine.step(info);

    EXPECT_EQ(0.0f, engine.energyEngine.get());
    EXPECT_EQ(EngineSimple::EngineState::OFF, engine.state);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_IGNITION]);

    // -- throttle should switch the motor on
    // cranking
    signals.reset();
    signals[SignalType::ST_GEAR] = 0;
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;

    engine.step(info);

    EXPECT_EQ(EngineSimple::EngineState::CRANKING, engine.state);
    EXPECT_EQ(RCSIGNAL_MAX, signals[SignalType::ST_IGNITION]);

    // running
    signals.reset();
    signals[SignalType::ST_GEAR] = 0;
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_TRUE;

    info.deltaMs = 1000U;  // cranking time
    engine.step(info);

    EXPECT_EQ(EngineSimple::EngineState::ON, engine.state);
    EXPECT_NEAR(engine.idleManager.getRPM(), engine.getRPM(), 10);
    EXPECT_EQ(RCSIGNAL_MAX, signals[SignalType::ST_IGNITION]);
    EXPECT_NEAR(signals[SignalType::ST_RPM], engine.getRPM(), 10);

    // -- the motor should switch off eventually
    for (int i = 0; i < 80; i++) {
        signals.reset();
        signals[SignalType::ST_GEAR] = 0;
        signals[SignalType::ST_THROTTLE] = RCSIGNAL_NEUTRAL;
        info.deltaMs = 20;

        engine.step(info);

        /*
        std::cout << "stat: RPM: " << engine.getRPM() <<
            " gear: " << static_cast<int>(engine.gearCurrent) <<
            " gearNext: " << static_cast<int>(engine.gearNext) <<
            " gear state: " << static_cast<int>(engine.gearState) <<
            " speed: " << (engine.energyVehicle.speed(engine.massVehicle) / (M_PI * engine.wheelDiameter)) <<
            " throttle: " << signals[SignalType::ST_THROTTLE] <<
            " idle: " << engine.idleTimeMs <<
            std::endl;
        */
    }

    EXPECT_EQ(EngineSimple::EngineState::OFF, engine.state);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_IGNITION]);
    EXPECT_EQ(RCSIGNAL_NEUTRAL, signals[SignalType::ST_THROTTLE]);
    EXPECT_GT(engine.idleManager.getRPM(), engine.getRPM());
}

/** Unit test for EngineGear RPM/throttle
 *
 *  This is a module test for the whole scenario of accelerating
 *  an deccelerating.
 */
TEST(EngineGearTest, RPM) {

    EngineGear engine;
    engine.crankingTimeMs = 1;
    engine.idleManager = rcEngine::Idle(500, 500, 0, 0, 10);
    engine.gears.set({3.0f, 2.0f, 1.3f, 1.0f, 0.0f}); // four gears
    engine.rpmMax = 1000u;
    engine.rpmShift = 600;
    engine.gearDecouplingTime = 0;
    engine.gearCouplingTime = 0;
    engine.gearDoubleDeclutchTime = 0;
    engine.massEngine = 10.0f;
    engine.massVehicle = 30.0f;
    engine.maxPower = 5000.0f;
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

    // cranking
    signals.reset();
    signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
    info.deltaMs = 10U;
    engine.step(info);

    // running
    signals.reset();
    signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
    signals[SignalType::ST_THROTTLE] = RCSIGNAL_TRUE;
    info.deltaMs = 3U;
    engine.step(info);

    EXPECT_NEAR(500, signals[SignalType::ST_RPM], 10);

    // -- bring throttle up
    for (int i = 0; i < 120; i++) {
        signals.reset();
        signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
        signals[SignalType::ST_THROTTLE] = RCSIGNAL_MAX;
        info.deltaMs = 20U;
        engine.step(info);

        /* for debugging you might want to switch it on
        std::cout << "stat: RPM: " << engine.getRPM() <<
            " gear: " << static_cast<int>(engine.gearCurrent) <<
            " gearNext: " << static_cast<int>(engine.gearNext) <<
            " gear state: " << static_cast<int>(engine.gearState) <<
            " speed: " << (engine.energyVehicle.speed(engine.massVehicle) / (M_PI * engine.wheelDiameter)) <<
            " throttle: " << signals[SignalType::ST_THROTTLE] <<
            std::endl;
        */
    }

    EXPECT_LT(3, signals[SignalType::ST_GEAR]);
    // std::cout << "--- decelerating " << std::endl;

    // -- bring throttle down
    for (int i = 0; i < 100; i++) {
        signals.reset();
        signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
        signals[SignalType::ST_THROTTLE] = RCSIGNAL_NEUTRAL;
        signals[SignalType::ST_ENGINE_LOAD] = 10;  // to brake faster
        info.deltaMs = 20U;
        engine.step(info);

        /* for debugging you might want to switch it on
        std::cout << "stat: RPM: " << engine.getRPM() <<
            " gear: " << static_cast<int>(engine.gearCurrent) <<
            " gearNext: " << static_cast<int>(engine.gearNext) <<
            " gear state: " << static_cast<int>(engine.gearState) <<
            " speed: " << (engine.energyVehicle.speed(engine.massVehicle) / (M_PI * engine.wheelDiameter)) <<
            " throttle: " << signals[SignalType::ST_THROTTLE] <<
            std::endl;
        */
    }

    EXPECT_GT(2, signals[SignalType::ST_GEAR]);
    EXPECT_GT(1000, signals[SignalType::ST_RPM]);
}


/** Unit test for EngineGear speed
 *
 *  This is a module test for the speed manager, checking
 *  if it's working.
 */
TEST(EngineGearTest, Speed) {

    EngineGear engine;
    engine.engineType = EngineSimple::EngineType::PETROL;
    engine.crankingTimeMs = 1;
    engine.idleManager = rcEngine::Idle(500, 500, 0, 0, 10);
    engine.gears.set({3.0f, 2.0f, 1.3f, 1.0f, 0.0f}); // four gears
    engine.rpmMax = 1000u;
    engine.rpmShift = 600;
    engine.gearDecouplingTime = 0;
    engine.gearCouplingTime = 0;
    engine.gearDoubleDeclutchTime = 0;
    engine.massEngine = 10.0f;
    engine.massVehicle = 30.0f;
    engine.maxPower = 5000.0f;
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

    // -- Accelerating
    for (int i = 0; i < 120; i++) {
        signals.reset();
        signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
        signals[SignalType::ST_SPEED] = 1000;
        info.deltaMs = 20U;

        /*
        if (i == 100)
            raise(SIGINT);
            */

        engine.step(info);

        /* for debugging you might want to switch it on
        std::cout << "stat: RPM: " << engine.getRPM() <<
            " gear: " << static_cast<int>(engine.gearCurrent) <<
            " gearNext: " << static_cast<int>(engine.gearNext) <<
            " gear state: " << static_cast<int>(engine.gearState) <<
            " throttle: " << signals[SignalType::ST_THROTTLE] <<
            " speed: " << signals[SignalType::ST_SPEED] <<
            std::endl;
        */
    }

    EXPECT_LT(3, signals[SignalType::ST_GEAR]);
    EXPECT_LT(600, signals[SignalType::ST_SPEED]);

    // std::cout << "--- decelerating " << std::endl;

    // -- bring throttle down
    for (int i = 0; i < 100; i++) {
        signals.reset();
        signals[SignalType::ST_IGNITION] = RCSIGNAL_MAX;
        signals[SignalType::ST_SPEED] = 0;
        signals[SignalType::ST_ENGINE_LOAD] = 10;  // to brake faster
        info.deltaMs = 20U;
        engine.step(info);

        /* for debugging you might want to switch it on
        std::cout << "stat: RPM: " << engine.getRPM() <<
            " gear: " << static_cast<int>(engine.gearCurrent) <<
            " gearNext: " << static_cast<int>(engine.gearNext) <<
            " gear state: " << static_cast<int>(engine.gearState) <<
            " throttle: " << signals[SignalType::ST_THROTTLE] <<
            " speed: " << signals[SignalType::ST_SPEED] <<
            std::endl;
        */
    }

    EXPECT_GT(1, signals[SignalType::ST_GEAR]);
    EXPECT_GT(1000, signals[SignalType::ST_RPM]);
    // since there is no braking happening, the speed
    // of the vehicle will stay high
    EXPECT_GT(200, signals[SignalType::ST_SPEED]);
}

