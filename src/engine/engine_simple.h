/**
 *  This file contains definition for the engine/vehicle simulation classes
 *  with the RC_Engine project.
 *
 *  @file
*/

#ifndef _RC_ENGINE_SIMPLE_H_
#define _RC_ENGINE_SIMPLE_H_

#include "proc.h"
#include "signals.h"

#include "engine_idle.h"  // for Idle

#include <cmath> // for sqrtf

class EngineSimpleTest_Energy_Test;
class EngineSimpleTest_StepEngine_Test;
class EngineSimpleTest_RPM_Test;
class EngineSimulator;

/** Namespace containing engine related procs. */
namespace rcEngine {

/** Helper class for handling energy
 *
 *  Energy in Joule
 *
 *  We are calculating the energy of the engine system
 *  using this formula:
 *
 *      E = m * v^2 (m in kg, v in m/s, E in j)
 *      v = sqrt(2 * E * m)
 */
struct Energy {

    /** Energy in Joule */
    float energy;

    float get() const {
        return energy;
    }

    /** Sets the energy
     *
     *  @param[in] value The new energy.
     */
    void set(float value) {
        energy = value;
        if (energy < 0.0f) {
            energy = 0.0f;
        }
    }

    /** Adds (or removes) energy.
     *
     *  Ensures that energy is never negative.
     *
     *  @param[in] value The energy to add or substract in Watts
     */
    void add(float value) {
        energy += value;
        if (energy < 0.0f) {
            energy = 0.0f;
        }
    }

    /** Returns the current kinetic energy from v (m/s)

     *  @param[in] mass Mass in kg
     *  @param[in] v in m/s
     */
    static float energyFromSpeed(const float& v, const float& mass) {
        return 0.5f * mass * (v * v);
    }

    /** Returns the speed from the given energy and mass.
     *
     *  @param[in] mass Mass in kg
     *  @return v in m/s
     */
    float speed(const float& mass) const {
        return sqrtf(2.0f * energy / mass);
    }
};

/** This class simulates an engine.
 *
 *  The simple engine only simulates an engine with
 *  inertia.
 *  No gear, braking or reverse.
 *
 *  The simple engine can be used for airplanes and
 *  boats. (Steam engine and jet turbines too)
 *
 *  The simulation covers the following parameters:
 *
 *  Inputs used are:
 *
 *  - rcSignals::SignalType::ST_IGNITION ()
 *  - rcSignals::SignalType::ST_THROTTLE (only positive)
 *  - rcSignals::SignalType::ST_ENGINE_LOAD
 *
 *  Outputs are:
 *
 *  - rcSignals::SignalType::ST_RPM
 *  - rcSignals::SignalType::ST_THROTTLE (actual throttle including, e.g. idle throttle)
 *


 */
class EngineSimple: public rcProc::Proc {
    protected:
        /** This enum is used when determining the troque curve
         *  for the engine power calculation.
         */
        enum class EngineType : uint8_t {
            ELECTRIC,  ///< Electic vehicle. Has power at zero RPM
            DIESEL,
            PETROL,
            PETROL_TURBO, ///< petrol engine with turbo
            STEAM,   ///< as in steam engine
            TURBINE, ///< as in jet turbine
        };

        /** Enum describing the state of the engine. */
        enum class EngineState : uint8_t {
            OFF,  ///< The engine is off (ignition is off)
            CRANKING,  ///< The engine is cranking. In this state the idle RPM is not guaranteed
            ON  ///< Engine is on, at least at idle RPM.
        };

        // -- configuration
        EngineType engineType;  ///< The engine type determines the power curve
        rcSignals::TimeMs crankingTimeMs;  ///< How long the engine stays in the cranking state (in ms)
        float massEngine;  ///< Mass used for RPM calculation. The more mass, the slower an engine will accelerate and deccelerate. This is just the engine and the unit is synthetic since we are using a rotating mass.
        float maxPower;  ///< maximum engine power in Watts.
        uint16_t rpmMax;  ///< maximum motor revolutions for the power curves in revolutions per minute. Must not be 0f

        Idle idleManager;  ///< the component handling stable idle RPM for us.

        // -- internal state

        /** Kinetic energy of the engine in Joule.
         *
         *  This energy is mostly the engine rotation
         *  and used when calculating it.
         *
         *  It is connected to the EngineGear::energyVehicle
         *  when the transmission is connected.
         */
        mutable Energy energyEngine;

        rcSignals::TimeMs stepTimeMs;  ///< the time in the current step.
        EngineState state;

        /** Returns the current power for the engine in Watt.
         *
         *  Looks up the power in the relevant power curves.
         *
         *  For a quick and dirty throttle we take the weighted sum
         *  of the motor throttle and the zeroThrottle curve.
         *
         *  This ensures that the power output is always negative at zero
         *  throttle.
         *
         *  @param[in] rpm The current RPM of the engine.
         *  @param[in] throttleRatio The ratio of the total available power,
         *    e.g. 0.5 for half throttle.
         *  @param[in] ignition If we have ignition. Some engine have extra braking
         *    force without ignition.
         *
         *  @returns The power produced by the engine in Watt.
         *    Negative if the engine is braking.
         */
        virtual float getPower(float rpm, float throttleRatio, bool ignition) const;

    protected:
        /** Get engine RPM.
         *
         *  @returns engine RPM.
         */
        float getRPM() const;

        /** Set engine RPM.
         *
         *  This is similar to setting the system energy.
         *  It is used to set a specific idle RPM.
         *
         *  @param[in] RPM The RPM to set.
         */
        void setRPM(float RPM);

        /** Determines the ignition signal.
         *
         *  Using the existing signals, tries to create an ignition signal.
         *
         *  @param[in] signals The input signals, mainly to get the initial ignition and
         *      the throttle as a fallback.
         *  @returns In addition to setting ignition in the info.signals also returns it.
         */
        virtual rcSignals::RcSignal getIgnition(const rcSignals::Signals& signals);

        /** Determines the throttle signal.
         *
         *  This might include a couple of steps:
         *  - creating throttle from speed signal
         *  - handling gear throttle increase/reduction
         *  - idle throttle
         *
         *  the function might have other side effects
         *
         *  @param[in] throttleIn The original throttle. Might be RCSIGNAL_INVALID.
         *  @param[in,out] info Contiains input signals, but others might also be modified.
         *  @returns In addition to setting throttle in the info.signals also returns it.
         *    The returned throttle might be negative or RCSIGNAL_INVALID.
         */
        virtual rcSignals::RcSignal getThrottle(
            rcSignals::RcSignal throttleIn,
            const rcProc::StepInfo& info);


        /** Steps the engine statemachine (state)
         *
         *  The following image shows the simple engine state machine.

        @startuml
        OFF : engine is off
        CRANKING : engine is starting
        ON : engine is running

        [*] --> OFF
        OFF --> CRANKING : ignition on
        CRANKING --> OFF : ignition off
        CRANKING --> ON : crankingTimeMs passed
        ON --> OFF : ignition off
        ON --> OFF : RPM < idle RPM / 4
        @enduml

        */
        void stepEngine(rcSignals::TimeMs deltaMs, rcSignals::RcSignal ignition);

    public:
        /** The constructor for EngineSimple.
         *
         *  This constructor set's values for a Great Western Railway 4900 Class
         *  locomotive.
         *
         *  Wheel diameter for that locomotive: 0.914 m
         */
        EngineSimple();

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const EngineSimple&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, EngineSimple&);

        friend EngineSimpleTest_Energy_Test;
        friend EngineSimpleTest_StepEngine_Test;
        friend EngineSimpleTest_RPM_Test;
        friend EngineSimulator;
};


} // namespace

#endif // _RC_ENGINE_SIMPLE_H_
