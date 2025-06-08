/**
 *  This file contains definition for the engine/vehicle simulation classes
 *  with the RC_Engine project.
 *
 *  @file
*/

#include "engine_gear.h"
#include "signals.h"

#include <cmath>
#include <algorithm>  // for sort and clamp

using namespace rcSignals;

namespace rcEngine {

// --- GearCollection

GearCollection::GearCollection() :
            gearRatios{2.3f, 1.7f, 1.3f, 1.0f, 0.0f},
            numGears(0) {
    updateGearList();
}

GearCollection GearCollection::forwardGears() const {
    GearCollection newGears = *this;
    for (float &ratio : newGears.gearRatios) {
        if (ratio < 0.0f) {
            ratio = 0.0f;
        }
    }
    newGears.updateGearList();
    return newGears;
}

GearCollection GearCollection::rearGears() const {
    GearCollection newGears = *this;
    for (float &ratio : newGears.gearRatios) {
        if (ratio > 0.0f) {
            ratio = 0.0f;
        } else {
            ratio = -ratio;
        }
    }
    newGears.updateGearList();
    return newGears;
}

void GearCollection::updateGearList() {
    // filter out negatives and sort reverse order
    auto gearRatiosSorted = gearRatios;
    std::sort(gearRatiosSorted.begin(), gearRatiosSorted.end());

    gearRatios = {0.0f};

    // copy the correct ones and count them.
    numGears = 0;
    auto pos = gearRatios.begin();

    // negatives first
    auto posSorted = gearRatiosSorted.begin();
    while ((pos != gearRatios.end()) && (*posSorted < 0.0f)) {
        *pos++ = *posSorted++;
        numGears++;
    }

    // positives next
    posSorted = gearRatiosSorted.end() - 1;
    while ((pos != gearRatios.end()) && (*posSorted > 0.0f)) {
        *pos++ = *posSorted--;
        numGears++;
    }
}

float GearCollection::get(int8_t index) const {

    if (numGears < 1) {
        return 1.0f;

    } else {
        index = std::clamp(index,
            static_cast<int8_t>(0),
            static_cast<int8_t>(numGears - 1));

        return gearRatios[index];
    }
}

// ---- EngineGear

EngineGear::EngineGear() :
            massVehicle(5000),
            wheelDiameter(0.7f),
            rpmShift(900u),
            gearDecouplingTime(200),
            gearCouplingFactor(150u),
            gearDoubleDeclutch(false),
            idleTimeMs(0U),
            gearCurrent(0),
            gearNext(0),
            gearState(GearState::STARTING),
            gearStepTime(0u),
            speedMax(1.0f)
             {

    // override values from EngineSimple
    engineType = EngineType::DIESEL;
    crankingTimeMs = 1000u;
    offTimeMs = 5000.0f;
    massEngine = 80.0f;
    maxPower = 370000.0f;  // 500hp
    rpmMax = 1800.0f,

    speedMax = getSpeedMax();
}

static float sqr(float a) {
    return a * a;
}

float EngineGear::getRotationRatio(const int8_t gear) const {
    if (gear > 0 && gears.size() > 0) {
        if (wheelDiameter > 0.0f) {
            return gears.get(gear - 1) / (wheelDiameter * M_PI);
        }
    }

    if (wheelDiameter > 0.0f) {
        return 1.0f / (wheelDiameter * M_PI);
    }

    return 1.0f;
}

float EngineGear::vehicleEnergyFactor(const int8_t gear) const {
    return massVehicle / (massEngine * sqr(getRotationRatio(gear)));
}

float EngineGear::maxPowerTransfer() const {
    if (gears.size() == 0) {
        return maxPower * 10.0f;
    }
    if (gearCurrent == 0) {
        return 0.0f;
    }

    switch (gearState) {
    case GearState::STARTING:
        return 0.0f;
    case GearState::DOUBLE_CLUTCH:
        if (rpmForGear(gearNext) > getRPM()) {
            return 0.0f;  // we throttle up only the engine
        } else {
            return maxPower * 0.5f;  // we slow down the engine
        }
    case GearState::COUPLING:
        return maxPower * static_cast<uint8_t>(gearCouplingFactor) / 100.0f;
    case GearState::COUPLED:
        return maxPower * 10.0f;
    case GearState::DECOUPLING:
        return 0.0f;
    default:
        return 0.0f;
    }
}


void EngineGear::distributeEnergy(const float energyEngineMin, const float energyEngineMax, const float maxEnergyTransfer) {

    // we assume a gear when distributing
    // in gear 0 we assume we want to distribute it correctly for gear 1
    int8_t adjustedGear = std::max(gearCurrent, static_cast<int8_t>(1));

    float disFactor = vehicleEnergyFactor(adjustedGear);

    float energy = energyEngine.get() + energyVehicle.get();

    // this would be the perfect distribution with
    // wheels an engine rotation connected via gear
    float energyEnginePerfect = energy / (1.0f + disFactor);

    // distribute according to the coupling factor:
    float deltaEnergy = energyEnginePerfect - energyEngine.get();

    // Limit the power transfered through the clutch
    if (deltaEnergy > 0.0f) {
        if (deltaEnergy > maxEnergyTransfer) {
            deltaEnergy = maxEnergyTransfer;
        }
    } else {
        if (deltaEnergy < -maxEnergyTransfer) {
            deltaEnergy = -maxEnergyTransfer;
        }
    }

    // kind of a smart clutch.
    // we move energy between engine and vehicle
    // only if energyEngineMin and energyEngineMax is not exceeded.
    if ((deltaEnergy < 0.0f) &&
        ((energyEngine.get() + deltaEnergy) < energyEngineMin)) {

        deltaEnergy = energyEngineMin - energyEngine.get();

    } else if ((deltaEnergy > 0.0f) &&
        ((energyEngine.get() + deltaEnergy) > energyEngineMax)) {

        deltaEnergy = energyEngineMax - energyEngine.get();
    }

    energyEngine.add(deltaEnergy);
    energyVehicle.add(-deltaEnergy);
}


float EngineGear::rpmForGear(int8_t gear) const {

    if (gear == 0) {
        return getRPM();  // gear 0 means disconnected, so return current RPM
    }

    float disFactor = vehicleEnergyFactor(gear);
    float energy = energyEngine.get() + energyVehicle.get();

    Energy perfectEnergyEngine(energy / (1.0f + disFactor));
    return perfectEnergyEngine.speed(massEngine) * 60.0f;
}


bool EngineGear::wantFaster(const rcSignals::Signals& signals) {
    auto throttle = signals[SignalType::ST_THROTTLE];
    auto brake = signals.get(SignalType::ST_BRAKE);

    if (brake != RCSIGNAL_INVALID) {
        if (brake > RCSIGNAL_EPSILON) {
            return false;
        }
    }

    if (throttle != RCSIGNAL_INVALID) {
        return throttle > RCSIGNAL_EPSILON;
    } else {
        return signals[SignalType::ST_SPEED] > relativeSpeed();
    }
}


int8_t EngineGear::chooseGear(bool faster) {

    float rpmTarget;
    if (faster) {
        rpmTarget = rpmShift;
    } else {
        rpmTarget = idleManager.getRPM();
    }

    // downshift to gear 0 if vehicle is stopped
    if ((!faster) && (energyVehicle.get() == 0.0f)) {
        return 0;
    }

    // the current or dedicated next gear get's a bonus
    const float rpmBonus = std::abs(rpmShift - idleManager.getRPM()) * 0.2f;

    for (int8_t gear = gears.size(); gear > 1; gear--) {

        float rpmAfterShift = rpmForGear(gear);
        if ((gear == gearCurrent) || (gear == gearNext)) {
            rpmAfterShift += rpmBonus;
        }

        if ((rpmAfterShift >= rpmTarget) &&
            (rpmAfterShift < rpmMax)) {
            return gear;
        }
    }

    return 1;
}


void EngineGear::stepGear(TimeMs deltaTime, bool shift) {

    gearStepTime += deltaTime;

    switch (gearState) {
    case GearState::STARTING:
        if (getRPM() >= rpmShift) {
            gearState = GearState::COUPLING;
            gearCurrent = gearNext;

        // fall back to neutral gear if we arent't already
        } else if (gearNext == 0) {
            gearCurrent = gearNext;
        }
        break;

    case GearState::DOUBLE_CLUTCH:
        if (gearNext == 0) {
            gearState = GearState::STARTING;

        } else if (std::abs(rpmForGear(gearCurrent) - getRPM()) < 10.0f) {
            gearState = GearState::COUPLING;
        }
        break;

    case GearState::COUPLING:
        if (shift) {  // we changed our mind.
            gearState = GearState::DECOUPLING;
            gearStepTime = 0;

        } else if (std::abs(rpmForGear(gearCurrent) - getRPM()) < 10.0f) {
            gearState = GearState::COUPLED;
        }
        break;

    case GearState::COUPLED:
        if (shift) {
            gearStepTime = 0;
            gearState = GearState::DECOUPLING;
        }
        break;

    // this is where we are depressing the clutch
    case GearState::DECOUPLING:
        if (gearStepTime >= gearDecouplingTime) {

            gearStepTime -= gearDecouplingTime;
            gearCurrent = gearNext;

            if (gearCurrent == 0) {
                gearState = GearState::STARTING;

            } else if (!gearDoubleDeclutch) {  // jump over DOUBLE_CLUTCH state
                gearState = GearState::COUPLING;

            } else {
                gearState = GearState::DOUBLE_CLUTCH;
            }
        }
        break;

    default:
        gearState = GearState::DOUBLE_CLUTCH;
    }
}


float EngineGear::relativeSpeed() {
    float speedCurrent = energyVehicle.speed(massVehicle);
    if (speedMax == 0.0f) {
        return RCSIGNAL_INVALID;
    }
    return speedCurrent / speedMax * RCSIGNAL_MAX;
}


float EngineGear::getSpeedMax() const {

    float minGearRatio = 1.0f;
    // gears are sorted. the last gear has the lowest ratio
    if (gears.size() > 0) {
        minGearRatio = gears.get(gears.size() - 1);
    }

    return rpmMax / 60.0f *
        (wheelDiameter * M_PI) / minGearRatio;
}


rcSignals::RcSignal EngineGear::getIgnition(const rcSignals::Signals& signals) {
    auto ignition = signals[SignalType::ST_IGNITION];

    if (ignition == RCSIGNAL_INVALID) {
        // note: negative throttle does not start the motor (EngineReverse provides positive)
        // note2: invalid signals don't start the engine either.
        const bool wantIgnition =
            (signals.get(SignalType::ST_THROTTLE, RCSIGNAL_NEUTRAL) >
                rcSignals::RCSIGNAL_EPSILON) ||
            (signals.get(SignalType::ST_SPEED, RCSIGNAL_NEUTRAL) >
                rcSignals::RCSIGNAL_EPSILON);

        if (state == EngineState::OFF) {
            if (wantIgnition) {
                ignition = RCSIGNAL_MAX;
            } else {
                ignition = RCSIGNAL_NEUTRAL;
            }
        } else {
            if (wantIgnition || (idleTimeMs < offTimeMs)) {
                ignition = RCSIGNAL_MAX;
            } else {
                ignition = RCSIGNAL_NEUTRAL;
            }
        }

        // don't turn ignition off if the vehicle is still
        // coasting along.
        if (wantIgnition || (energyVehicle.get() > 0.0)) {
            idleTimeMs = 0u;
        }
    }

    return ignition;
}


RcSignal EngineGear::getThrottle(
            rcSignals::RcSignal throttle,
            const rcProc::StepInfo& info) {

    Signals& signals = *(info.signals);

    // -- gears (double-declutch, switching gears
    switch (gearState) {
    case GearState::STARTING:
        // keep original throttle
        break;

    case GearState::DOUBLE_CLUTCH:
        if (rpmForGear(gearNext) > getRPM()) {
            throttle = RCSIGNAL_MAX / 2;  // double-declutch, Zwischengas
        } else {
            throttle = RCSIGNAL_NEUTRAL;
        }
        break;

    case GearState::COUPLING:
    case GearState::COUPLED:
        // keep original throttle
        break;

    case GearState::DECOUPLING:
        throttle = RCSIGNAL_NEUTRAL;
        break;

    default:
        ; // keep original throttle
    }

    // -- throttle from speed
    auto speedTarget = signals[SignalType::ST_SPEED];
    if ((throttle == RCSIGNAL_INVALID) &&
        (speedTarget != RCSIGNAL_INVALID)) {

        speedManager.step(info.deltaMs, relativeSpeed(), speedTarget, &throttle);
    }

    return EngineSimple::getThrottle(throttle, info);
}


void EngineGear::start() {

    gearNext = 0;
    gearCurrent = 0;
    gearState = GearState::STARTING;
    gearStepTime = 0;
    idleTimeMs = 0;

    energyVehicle.set(0.0f);
    speedMax = getSpeedMax();

    EngineSimple::start();
}


void EngineGear::step(const rcProc::StepInfo& info) {

    Signals& signals = *(info.signals);

    if (energyVehicle.get() == 0.0f) {
        idleTimeMs += info.deltaMs;
    } else {
        idleTimeMs = 0;
    }

    // -- handle gear
    const auto gearOrig = signals[SignalType::ST_GEAR];
    const auto faster = wantFaster(signals);
    if (gearOrig == RCSIGNAL_INVALID) {
        gearNext = chooseGear(faster);
    } else {
        gearNext = std::clamp(
          static_cast<int8_t>(gearOrig),
          static_cast<int8_t>(0),
          static_cast<int8_t>(gears.size()));
    }
    bool shift = (gearNext != gearCurrent);
    stepGear(info.deltaMs, shift);

    // -- call EngineSimple
    EngineSimple::step(info);

    // -- distribute energy between engine and vehicle

    // we want to keep a minimum RPM
    // for gear 1 we want to allow the engine to spool up to idle RPM
    // for all other gears we need to allow the engine to slow down
    //   below the downshift RPM (idle * 0.85)
    float rpmMin = idleManager.getRPM();
    if (gearCurrent > 1) {
        rpmMin = (idleManager.getRPM() * 0.7f);
    }

    float energyMinRPM = Energy::energyFromSpeed(rpmMin / 60.0f, massEngine);
    float energyMaxRPM = Energy::energyFromSpeed(rpmMax / 60.0f, massEngine);

    float maxEnergyTransfer = maxPowerTransfer() * info.deltaMs / 1000.0f;
    distributeEnergy(energyMinRPM, energyMaxRPM, maxEnergyTransfer);

    // -- write back signals
    signals[SignalType::ST_GEAR] = gearCurrent;
    signals[SignalType::ST_RPM] = getRPM(); // after distributeEnergy() the RPM changed
    signals[SignalType::ST_SPEED] = std::clamp(
        relativeSpeed(),
        static_cast<float>(-RCSIGNAL_MAX),
        static_cast<float>(RCSIGNAL_MAX));
}

} // namespace

