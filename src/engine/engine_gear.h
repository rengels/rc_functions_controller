/**
 *  This file contains definition for the engine/vehicle simulation classes
 *  with the RC_Engine project.
 *
 *  @file
*/

#ifndef _RC_ENGINE_GEAR_H_
#define _RC_ENGINE_GEAR_H_

#include "engine_simple.h"
#include "engine_speed.h"
#include <algorithm>

class GearCollectionTest_UpdateGearList_Test;

class EngineGearTest_RotationRatio_Test;
class EngineGearTest_VehicleEnergyFactor_Test;
class EngineGearTest_CouplingFactor_Test;
class EngineGearTest_DistributeEnergy_Test;
class EngineGearTest_RpmForGear_Test;
class EngineGearTest_ChooseGear_Test;
class EngineGearTest_Energy_Test;

class EngineGearTest_Ignition_Test;

class EngineGearTest_SelectGear_Test;
class EngineGearTest_RPM_Test;
class EngineGearTest_Speed_Test;

class EngineGearTest_GetMass_Test;
class EngineGearTest_Energy_Test;

class EngineSimulator;

namespace rcEngine {

/** This class represents a collection of gears.
 *
 *  We use this:
 *  - special serialization functions for this type to prevent
 *    transmitting 25 floats.
 *  - gear sorting is build in.
 *  - easier to implement a special widget for the UI.
 *
 */
class GearCollection {
    public:
        /** Maximum number of gears.
         *
         *  Heavy duty truck transmission have that much gears:
         *
         *  @see https://paccarpowertrain.com/products/tx-18/?v=92a31fc033f7
         */
        static constexpr int8_t NUM_GEARS = 25;

    private:
        /** Gear ratios for the forward gears.
         *
         *  EngineGear is still only handling forward drives, that
         *  means this list should only contain positive ratios.
         *
         *  The higher the number, the lower the gear.
         *  e.g. 5.0 means that the engine does five revolutions for one
         *  revolution of the wheel.
         *
         *  We keep the gear ratios sorted in decending order.
         */
        std::array<float, NUM_GEARS> gearRatios;

        /** The number of valid gears in the gear ratios list. This is calculated by updateGearList() */
        int8_t numGears;

    public:
        GearCollection();

        void set(std::array<float, NUM_GEARS> valGearRatios) {
            gearRatios = valGearRatios;
            updateGearList();
        }

        /** Returns the specified gear index.
         *
         *  With 0 gears we assume a gear ratio of 1.0f.
         *
         *  @param index The index of the gear ratio to be returned. Will be clamped.
         */
        float get(int8_t index) const;

        int8_t size() const {
            return numGears;
        }

        /** Returns new gears with all forward gears removed and all rear-gears changed to positive gears. */
        GearCollection forwardGears() const;

        /** Returns new gears with all rear gears removed. */
        GearCollection rearGears() const;

        /** This function sorts the gearRatios list and calculates numGears.
         *
         *  The gears are sorted:
         *  - negatives increasing
         *  - positives decreasing
         *  - zeroes
         */
        void updateGearList();

        friend GearCollectionTest_UpdateGearList_Test;
};

/** This class simulates a vehicle with engine, gear and mass but no reverse driving.
 *
 *  It adds to the EngineSimple implementation
 *  by simulating a vehicle (with mass) and transmission.
 *
 *  However it does not do
 *  - rear-gear, rear-driving or anything in that area
 *  - braking (except with the engine)
 *  - trailer
 *
 *  Two concepts are important:
 *
 *  Split energy
 *  ------------
 *
 *  The energy is split between engine and vehicle and handled
 *  separately.
 *  If the clutch is connected, also the energy is connected via
 *  the vehicleEnergyFactor().
 *  The distributeEnergy() function will re-balance this energy.
 *
 *  Gear change
 *  -----------
 *
 *  Gear changing is split into three phases:
 *
 *  - disengaging gear (takes a fixed gearDecouplingTime)
 *  - double-declutch (Zwischengas) (used the gearDoubleDeclutch time)
 *  - engaging gear (takes a fixed gearCouplingTime)
 *
 *  The double-declutch is only necessary for an unsynchronized gear.
 *
 *  In case of no gear (gears.size() == 0) we assume a gear ratio of 1.0
 *  and it's always connected.
 *
 *  RPMs
 *  ----
 *
 *  There are a lot of different RPMs being used in selecting gears
 *  and simulating the engine.
 *  Sorted in descending order:
 *
 *  - rpmMax : Maximum RPM for the power calculation (defined in EngineSimple)
 *  - rpmShift : RPM that we will try to hold at least when switching gears
 *  - rpmShift - hysterese : an internaly calculated RPM used for
 *    gear down shifting.
 *  - idle rpm: The rpm from the idle manager. The idle manager will
 *    actively regulate the throttle to hold this RPM.
 *  - idle rpm * 0.85: Used in chooseGear when downshifting
 *  - idle rpm * 0.7: Used in distribute energy when giving engergy to
 *    the engine.
 *
 *  The simulation covers the following parameters:
 *
 *  Inputs used are:
 *
 *  - rcSignals::SignalType::ST_IGNITION (will be generated if invalid)
 *  - rcSignals::SignalType::ST_THROTTLE
 *
 *  Outputs are:
 *
 *  - rcSignals::SignalType::ST_IGNITION (inherited from EngineSimple)
 *  - rcSignals::SignalType::ST_RPM (inherited from EngineSimple)
 *  - rcSignals::SignalType::ST_SPEED
 *  - rcSignals::SignalType::ST_GEAR
 *
 */
class EngineGear: public EngineSimple {
    protected:
        float massVehicle;   ///< Mass of the vehicle in kg

        rcSignals::TimeMs offTimeMs; ///< How long the engine stays running in case of no throttle and invalid ignition

        GearCollection gears;

        /** The diameter of the wheels.
         *
         *  This is part of the rotation ratio calculation and
         *  mostly influences the speed and the energy distribution
         *  between vehicle and engine.
         *
         *  in m (e.g. 20inch + tire == .7 m)
         */
        float wheelDiameter;

        /** This is the gear shift trigger RPM.
         *
         *  When shifting the engine RPM changes due to the different gear ratios.
         *  When accelerating you would like to keep a higher RPM to keep on
         *  accelerating.
         *  So (when not getting an explicit gear signal) the algorithm will
         *  shift only when the RPM after gear change is at least rpmShift.
         */
        uint16_t rpmShift;

        rcSignals::TimeMs gearDecouplingTime; ///< The time it takes to gradualy disengage the gear. Set to 0 for double-clutch
        rcSignals::TimeMs gearCouplingTime; ///< The time it takes to gradualy engage the gear. Set to 0 for double-clutch
        rcSignals::TimeMs gearDoubleDeclutchTime; ///< The time between disengaging and engaging used to bring the gear up to speed. Set to 0 for a synchronized gear.

        /** Kinetic energy of the vehicle in Joule
         *
         *  EngineSimple handles _energyEngine_.
         *  In case the transmission is decoupled, the vehicle energy and the
         *  engine energy are disconnected, so we need two values.
         *
         *  Always positive.
         */
        mutable Energy energyVehicle;

        /** Number of ms that we had to keep the idleRPM due to low throttle.
         *
         *  This is used for the engine-off/ignition time detection.
         */
        rcSignals::TimeMs idleTimeMs;

        /** The current gear.
         *
         *  Set by selectGear()
         *
         *  Note: EngineReverse will switch
         *  the gears around when switching driving direction, so this number
         *  is always positive.
         */
        int8_t gearCurrent;

        /** The gear that we want to switch to (if not equal to gearCurrent) */
        int8_t gearNext;

        /** Enum describing the state of the gear change. */
        enum class GearState : uint8_t {
            DECOUPLED,  ///< Gear is completely disconnected. We can do double-declutch at this point.
            COUPLING,  ///< We are in the process of connecting the gear.
            COUPLED,  ///< We are in the process of connecting the gear.
            DECOUPLING  ///< We are in the process of disconnecting the gear.
        };

        GearState gearState;  ///< The state of the gear statemachine.

        rcSignals::TimeMs gearStepTime; ///< The time in the current gear state.

        float speedMax;  ///< buffered result from getSpeedMax(). In m/s

        rcEngine::Speed speedManager;  ///< speed managing component.

    protected:

        /** The ratio of engine rotation vs speed
         *
         *  This includes the wheel circumfence and the gear ratio.
         *  rotations * ratio = distance
         *
         *  e.g. a ratio of 2 means that two engine rotations equate to 1 meter
         *  vehicle travel.
         *
         *  The ratio for \p gear 0 (disconnected) is 1 but it does not really
         *  mean anything.
         *
         *  @param[in] gear The gear for which the calcualtion should be done.
         */
        float getRotationRatio(int8_t gear) const;

        /** Returns the factor of vehicle vs. engine energy
         *
         *  If clutch/gear is engaged it doesn't actually matter
         *
         *  if we add to the engine energy or vehicle energy.
         *  However, when calculating the actual speed or
         *  diseangaging the gear we need to split them up in a way
         *  that keeps the engine RPM.
         *
         *  This function gives you the energy distribution factor which
         *  depends on the speed different of the engine vs. the vehicle.
         *
         *  engine_energy = total_energy / (1.0f + disFactor);
         *
         *  @param[in] gear The gear for which the calcualtion should be done.
         */
        float vehicleEnergyFactor(int8_t gear) const;

        /** Returns true if the clutch is considered engaged.
         *
         *  If the clutch is engaged, we distribute the energy between
         *  engine and vehicle.
         */
        bool clutchEngaged() const;

        /** This will distribute the energy between engine and vehicle
         *
         *  If the vehicle wheels and the engine are connected via
         *  gear and clutch, then the energy is also coupled with the
         *  vehicle energy factor.
         *
         *  This function will distribute the energy between
         *  engine and vehicle according to the vehicleEnergyFactor()
         *
         *  @param[in] energyEngineMin The minimum energy that we will
         *    allow the engine to have.
         *    So energy is moved in favor of the engine (min RPM).
         */
        void distributeEnergy(float energyEngineMin, float energyEngineMax);

        /** Calculates the RPM if the given \p gear would have been
         *  engaged.
         *
         *  @param[in] gear The gear for which the calcualtion should be done.
         */
        float rpmForGear(int8_t gear) const;

        /** Determine if, due to the input signals, we want to accelerate.
         *
         *  @param[in] signals The input signals, to determine if we want to accelerate.
         */
        bool wantFaster(const rcSignals::Signals& signals);

        /** This decides which gear to use right now.
         *
         *  Returns a gear that should be switched to next.
         *  \ref EngineGear::gearCurrent
         *
         *  It uses the initial throttle (the one that does not consider
         *  idle RPM keeping and all the other stuff) to determine
         *  if we want to go faster and thus a higher gear.
         *
         * @startuml
         * start
         * if (faster?) then (yes)
         *   :calculate upshift RPM;
         *   if (upshift RPM > shift RPM?) then (yes)
         *     :upshift;
         *     stop
         *   endif
         *   if (current rpm < shift RPM?) then (yes)
         *     :downshift;
         *     stop
         *   endif
         *   :keep gear;
         *   stop
         * else (no)
         *   :calculate upshift RPM;
         *   if (upshift RPM > idle RPM?) then (yes)
         *     :upshift;
         *     stop
         *   endif
         *   if (gear == 1) then (yes)
         *     if (vehicle is stopped) then (yes)
         *        :gear = 0;
         *        stop
         *     endif
         *   endif
         *   if (RPM < idle RPM * 0.85?) then (yes)
         *     :downshift;
         *     stop
         *   endif
         *   :keep gear;
         *   stop
         * endif
         * @enduml
         *
         *  @param[in] faster True if we want to accelerate the vehicle.
         *  @returns The new gear or gearCurrent in case of no change.
         */
        int8_t chooseGear(bool faster);

        /** Steps the gear statemachine.
         *
         *  Will try to engage \ref gearNext if not equal to \ref gearCurrent.
         *
         * @startuml
         * DECOUPLED : gear is disconnected\nmight give double-declutching\nthrottle
         * COUPLING : gear is currently coupling
         * DECOUPLING : gear is currently decoupling
         * COUPLED : gear is connected
         *
         * [*] --> STARTING
         * STARTING --> COUPLING : no clutch slipage
         * DECOUPLED --> STARTING : nextGear == 0
         * DECOUPLED --> COUPLING : gearDoubleDeclutchTime passed
         * COUPLING --> COUPLED : gearCouplingTime passed
         * COUPLED --> DECOUPLING : if gearCurrent != gearChoosen
         * DECOUPLING --> DECOUPLED : gearDecouplingTime passed
         * @enduml
         *
         * @param[in] deltaTime The time passed since the last call to \ref selectGear()
         */
        void selectGear(rcSignals::TimeMs deltaTime);

        /** Returns the relative speed scaled to 0-1000 relative to speedMax.
         *
         *  @returns The relative speed (but in float, since we need the
         *    precission for rcEngine::Speed::step();
         */
        float relativeSpeed();

        /** Calculates the maximum speed of the vehicle in m/s.
         *
         *  Uses the gear ratio and max RPM, not the actual
         *  available power vs. resistance.
         *
         *  The result of this function is buffered in \ref speedMax.
         */
        virtual float getSpeedMax() const;

        virtual rcSignals::RcSignal getIgnition(const rcSignals::Signals& signals) override;

        /** Determines the throttle signal.
         *
         *  This function overrides the one in EngineSimple.
         *  It consideres double-declutch (Zwischengas) if the gear
         *  is decoupled.
         *
         *  @param[in] throttleIn The original throttle.
         *  @param[in,out] info Input signals
         *  @returns The throttle.
         */
        virtual rcSignals::RcSignal getThrottle(
            rcSignals::RcSignal throttleIn,
            const rcProc::StepInfo& info);

    public:
        /** Constructor for EngineGear.
         *
         *  This constructor configures a generic truck with
         *  a number of gears and appropriate masses.
         */
        EngineGear();

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const EngineGear&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, EngineGear&);

        // our test friends
        friend EngineGearTest_RotationRatio_Test;
        friend EngineGearTest_VehicleEnergyFactor_Test;
        friend EngineGearTest_CouplingFactor_Test;
        friend EngineGearTest_DistributeEnergy_Test;
        friend EngineGearTest_RpmForGear_Test;
        friend EngineGearTest_ChooseGear_Test;
        friend EngineGearTest_Energy_Test;

        friend EngineGearTest_Ignition_Test;

        friend EngineGearTest_SelectGear_Test;
        friend EngineGearTest_RPM_Test;
        friend EngineGearTest_Speed_Test;

        friend EngineGearTest_GetMass_Test;

        friend EngineGearTest_Energy_Test;
        friend EngineSimulator;
};

} // namespace

#endif // _RC_ENGINE_GEAR_H_
