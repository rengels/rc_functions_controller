/**
 *  Implementation of the demo input functionality.
 *
 *  @file
*/

#ifndef _INPUT_DEMO_H
#define _INPUT_DEMO_H

#include "input.h"
#include "signals.h"

class EngineSimulator;

namespace rcInput {

/** This class simulates a driving scenario for a vehicle without actually
 *  reading any external inputs.
 *
 *  This class is also used in the engine simulation.
 */
class InputDemo : public Input {
    public:
        /** Type of demo. */
        enum class DemoType : uint8_t {
            /** A truck demo.
             *
             *  This is the most complex one since it relies
             *  on the Engine to create a lot of signals.
             *
             *  Some slow driving, honking and trailer hitching, then
             *  driving around for a couple of seconds.
             *
             *  This demo relies on the engine to
             *  - select the gear
             *  - braking
             *  - throttle (we use ST_SPEED)
             *  - ignition
             *
             */
            TRUCK = 0,

            /** A simple demo.
             *
             *  No steering or lights.
             *  Just coupling, accelerating and then braking with
             *  manual braking signal.
             *
             *  Can also be used for an airplane.
             */
            TRAIN,

            /** A car demo with revving.
             *
             *  All light switched on, engine revving and then
             *  accelerating full throttle.
             *
             *  This demo has manual gear signals (that's why
             *  revving even works).
             */
            CAR,

            /** Simple demo script just accelerating and deccelerating.
             */
            SIMPLE,
        };
    private:
        rcSignals::Signals mySignals;
        rcSignals::TimeMs myTime;
        uint16_t currentStep; ///< index into the demo script

        DemoType scriptType;


    public:
        InputDemo(DemoType typeVal = DemoType::TRUCK);
        virtual ~InputDemo();

        virtual void start() override;
        virtual void stop() override;
        virtual void step(const rcProc::StepInfo& info) override;

        friend SimpleOutStream& operator<<(::SimpleOutStream& out, const InputDemo&);
        friend SimpleInStream& operator>>(::SimpleInStream& in, InputDemo&);

        friend EngineSimulator;
};


} // namespace

#endif // _INPUT_DEMO_H
