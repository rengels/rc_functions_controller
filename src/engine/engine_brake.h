/**
 *  This file contains definition for the engine/vehicle simulation classes
 *  with the RC_Engine project.
 *
 *  @file
*/

#ifndef _RC_ENGINE_BRAKE_H_
#define _RC_ENGINE_BRAKE_H_

#include "engine_gear.h"

class EngineSimulator;

namespace rcEngine {

/** This class adds braking and air resistance to
 *  the inherited engine implementation
 *
 *  The simulation covers the following parameters:
 *
 *  Inputs used are (excluding the inputs from EngineGear):
 *
 *  - rcSignals::SignalType::ST_THROTTLE (optional)
 *  - rcSignals::SignalType::ST_BRAKE (optional)
 *
 *  Outputs are (excluding the outputs from EngineGear):
 *
 *  - rcSignals::SignalType::ST_BRAKE (optional)
 *
 */
class EngineBrake: public EngineGear {
    protected:
        /** The maximum power (in Watts) that the brake will create.
         *
         *  Good starting point would be 3 times the max engine power.
         */
        float brakePower;

        /** Flat resistance power of the vehicle.
         *
         *  This helps slowing down the vehicle in neutral gear.
         *  In Watts.
         */
        float resistance;

        /** Air resistance value in m^2
         *
         *  We are using the drag equation given here:
         *  https://en.wikipedia.org/wiki/Drag_equation
         *
         *  The air resistance as (A * Cd). Around 2.0 for a car.
         */
        float airResistance;

        /** Determines the brake signal.
         *
         *  Negative throttle is considered braking if ther is no
         *  valid brake signal.
         *
         *  @param[in,out] info Input signals
         *
         *  @returns The brake.
         */
        rcSignals::RcSignal getBrake(const rcProc::StepInfo& info);

        virtual float getSpeedMax() const override;

    public:
        EngineBrake();
        ~EngineBrake() {};

        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const EngineBrake&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, EngineBrake&);

        friend EngineSimulator;
};

} // namespace

#endif // _RC_ENGINE_BRAKE_H_
