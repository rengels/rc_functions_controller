/**
 *  This file contains definition for the engine/vehicle simulation classes
 *  with the RC_Engine project.
 *
 *  @file
*/

#ifndef _RC_ENGINE_REVERSE_H_
#define _RC_ENGINE_REVERSE_H_

#include "engine_brake.h"

class EngineReverseTest_SetDrivingState_Test;
class EngineReverseTest_Reverse_Test;

class ProcStorage;
class EngineSimulator;

namespace rcEngine {


/** This class adds reversing to the vehicle simulation
 *
 *  It adds to the EngineGear implementation
 *  the reversing feature.
 *
 *  The EngineReverse will handle backwards driving by only
 *  presenting forward signals to the underlaying engines.
 *
 *  The inputs for determining the driving direction are in order
 *  of preference:
 *
 *  - gear
 *  - throttle
 *  - speed
 *
 *  The simulation covers the following parameters:
 *
 *  Inputs used are (excluding the outputs from EngineGear):
 *
 *  - rcSignals::SignalType::ST_GEAR (optional)
 *  - rcSignals::SignalType::ST_THROTTLE (optional)
 *  - rcSignals::SignalType::ST_BRAKING (optional)
 *  - rcSignals::SignalType::ST_SPEED (optional)
 *
 *  Outputs are (excluding the outputs from EngineGear):
 *
 *  - rcSignals::SignalType::ST_THROTTLE (optional)
 *  - rcSignals::SignalType::ST_SPEED (optional)
 *
 */
class EngineReverse: public EngineBrake {
    protected:
        enum class DrivingState : uint8_t {
            STOPPED_FWD,
            STOPPED_BCK,
            FORWARD,
            BACKWARD
        };

        /** The current driving state. */
        DrivingState drivingState;

        /** The time we've been stopped (low vehicle energy) in ms. */
        rcSignals::TimeMs stoppedTimeMs;

        /** The delay between vehicle stopped and reversing. */
        rcSignals::TimeMs reverseDelayMs;

        /** Gear ratios for all gears. Rear gears have a negative value. Invalid gears have a value of 0. */
        GearCollection fullGears;

        /** Sets the new driving direction.
         *
         *  Sets the drivingState variable and
         *  switch the gears for the EngineGear class.
         */
        void setDrivingState(const DrivingState newState);

        /** Steps the driving statemachine.
         *
         * @startuml
         * STOPPED : vehicle is ready for driving
         * FORWARD : vehicle is driving forward
         * BACKWARD : vehicle is driving reverse
         *
         * [*] --> STOPPED_FWD
         * STOPPED_BCK --> STOPPED_FWD : throttle > 0
         * STOPPED_FWD --> STOPPED_BCK : throttle < 0
         * STOPPED_FWD --> FORWARD : speed > epsilon
         * STOPPED_BCK --> BACKWARD : speed < epsilon
         * FORWARD --> STOPPED_FWD : vehicle stopped, gear == 0 and\n signal == 0
         * FORWARD --> STOPPED_FWD : vehicle stopped, gear == 0 and\n reverseDelayMs time passed
         * BACKWARD --> STOPPED_BCK : vehicle stopped, gear == 0 and\n signal == 0
         * BACKWARD --> STOPPED_BCK : vehicle stopped, gear == 0 and\n reverseDelayMs time passed
         * @enduml
         *
         *  @param[in] signal The input signal, either throttle or speed.
         */
        void drivingStatemachine(rcSignals::RcSignal signal);

    public:
        EngineReverse();

        virtual void start() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const EngineReverse&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, EngineReverse&);

        // our test friends
        friend EngineReverseTest_SetDrivingState_Test;
        friend EngineReverseTest_Reverse_Test;

        friend ProcStorage;
        friend EngineSimulator;
};

} // namespace

#endif // _RC_ENGINE_REVERSE_H_
