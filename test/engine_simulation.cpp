/** Tool for simulating an engine with parameters.
 *
 *  Sometimes it's hard to figure out how an engine
 *  behaves with all the different paramenters.
 *
 *  This tool should help by providing a way to simulate
 *  the engine behaviour over time.
 *
 *
 *  Idea for the command line parsing: https://stackoverflow.com/questions/865668/parsing-command-line-arguments-in-c
 *  @file
 */

#include "proc.h"
#include "engine_simple.h"
#include "engine_gear.h"
#include "engine_brake.h"
#include "engine_reverse.h"
#include "input_demo.h"

#include <signal.h>

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <span>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

using namespace std;
using namespace rcSignals;
using namespace rcProc;
using namespace rcEngine;

string getCmdOption(std::span<char*> args, const std::string & option);
bool cmdOptionExists(std::span<char*> args, const std::string& option);

/** A class that is a friend of all engines, allowing us to simulate
 *  them and get access to internal values.
 */
class EngineSimulator {
public:

    /** A meassurement at a time point. */
    struct Meassurement {
        rcSignals::TimeMs time; ///< absolute simulation time
        std::string strState;
        rcSignals::RcSignal speedSig; ///< the speed signal

        rcSignals::RcSignal throttleOrig; ///< original throttle signal
        rcSignals::RcSignal throttle;     ///< throttle signal after proc

        rcSignals::RcSignal speedOrig;    ///< original speed signal
        rcSignals::RcSignal speed;        ///< speed signal after proc

        rcSignals::RcSignal brake;        ///< brake ratio

        float power;            ///< engine power in Watt.
        float rpm;              ///< motor revolutions rev per minute.
        float speedVal;         ///< calculated speed of the vehicle in m/s
        float energyEngine;     ///< kinetic energy of the engine in Joule
        float energyVehicle;    ///< kinetic energy of the vehicle in Joule
        int16_t gear;           ///< current selected gear
        std::string strGearState;
        std::string strDrivingState;
    };


    rcInput::InputDemo inputDemo;
    rcEngine::EngineSimple* engine; ///< the engine for the simulation. not necessarily a "simple".
    const po::variables_map& config;

    vector<Meassurement> meassurements;


    EngineSimulator(const po::variables_map& configVal) :
        engine(nullptr),
        config(configVal) {

        // generate input
        if (config.count("demo")) {
            auto demoType = config["demo"].as<string>();
            if (demoType == "truck") {
                inputDemo.scriptType = rcInput::InputDemo::DemoType::TRUCK;
            } else if (demoType == "train") {
                inputDemo.scriptType = rcInput::InputDemo::DemoType::TRAIN;
            } else if (demoType == "car") {
                inputDemo.scriptType = rcInput::InputDemo::DemoType::CAR;
            } else if (demoType == "simple") {
                inputDemo.scriptType = rcInput::InputDemo::DemoType::SIMPLE;
            } else {
                cerr << "Invalid parameter for --demo: " << demoType << endl;
                exit(1);
            }
        }

        // generate engine
        if (config.count("engine")) {
            auto engineType = config["engine"].as<string>();
            if (engineType == "simple") {
                engine = new rcEngine::EngineSimple();
            } else if (engineType == "gear") {
                engine = new rcEngine::EngineGear();
            } else if (engineType == "brake") {
                engine = new rcEngine::EngineBrake();
            } else if (engineType == "reverse") {
                engine = new rcEngine::EngineReverse();
            } else {
                cerr << "Invalid parameter for --engine: " << engineType << endl;
                exit(1);
            }
        } else {
            engine = new rcEngine::EngineReverse();
        }

        auto engineGear = dynamic_cast<rcEngine::EngineGear*>(engine);
        if ((inputDemo.scriptType == rcInput::InputDemo::DemoType::TRUCK) &&
            (engineGear == nullptr)) {
            cerr << "WARNING: Demo type \"truck\" only set's a speed and not a throttle. "
                "You will need at least proc \"brake\" to get usefull results." << endl << endl;
        }

        configureEngine();
    }


    ~EngineSimulator() {
        delete engine;
    }

    /** Set the param for the engine from the command line. */
    void configureEngine() {
        auto engineGear = dynamic_cast<rcEngine::EngineGear*>(engine);
        auto engineBrake = dynamic_cast<rcEngine::EngineBrake*>(engine);
        auto engineReverse = dynamic_cast<rcEngine::EngineReverse*>(engine);

        if (config.count("power")) {
            engine->maxPower = config["power"].as<float>();
        }
        if (config.count("rpmMax")) {
            engine->rpmMax = config["rpmMax"].as<float>();
        }
        if (config.count("rpmIdle")) {
            engine->idleManager = rcEngine::Idle(
                config["rpmIdle"].as<float>(),
                config["rpmIdle"].as<float>(),
                0,  // loadStart
                0,  // timeStart
                30);
        }
        if (config.count("massE")) {
            engine->massEngine = config["massE"].as<float>();
        }
        if (config.count("type")) {
            auto typeType = config["type"].as<string>();
            if (typeType == "electric") {
                engine->engineType = rcEngine::EngineSimple::EngineType::ELECTRIC;
            } else if (typeType == "diesel") {
                engine->engineType = rcEngine::EngineSimple::EngineType::DIESEL;
            } else if (typeType == "petrol") {
                engine->engineType = rcEngine::EngineSimple::EngineType::PETROL;
            } else if (typeType == "petrol_turbo") {
                engine->engineType = rcEngine::EngineSimple::EngineType::PETROL_TURBO;
            } else if (typeType == "steam") {
                engine->engineType = rcEngine::EngineSimple::EngineType::STEAM;
            } else if (typeType == "turbine") {
                engine->engineType = rcEngine::EngineSimple::EngineType::TURBINE;
            } else {
                cerr << "Invalid parameter for --type: " << typeType << endl;
                exit(1);
            }
        }

        if (engineGear) {
            if (config.count("massV")) {
                engineGear->massVehicle = config["massV"].as<float>();
            }
            if (config.count("wheel")) {
                engineGear->wheelDiameter = config["wheel"].as<float>();
            }
            if (config.count("rpmShift")) {
                engineGear->rpmShift = config["rpmShift"].as<float>();
            }
            if (config.count("timeCouple")) {
                engineGear->gearCouplingTime = config["timeCouple"].as<rcSignals::TimeMs>();
            }
            if (config.count("timeDecouple")) {
                engineGear->gearDecouplingTime = config["timeDecouple"].as<rcSignals::TimeMs>();
            }
            if (config.count("timeDoubleDeclutch")) {
                engineGear->gearDoubleDeclutchTime = config["timeDoubleDeclutch"].as<float>();
            }
        }

        if (engineBrake) {
            if (config.count("powerBrake")) {
                engineBrake->brakePower = config["powerBrake"].as<float>();
            }
            if (config.count("airResistance")) {
                engineBrake->airResistance = config["airResistance"].as<float>();
            }
        }

        if (config.count("gears")) {
            auto vGearRatios = config["gears"].as< vector<float> >();

            // read the gears form the string stream
            std::array<float, GearCollection::NUM_GEARS> gearRatios;
            std::fill(std::begin(gearRatios), std::end(gearRatios), 0.0f);
            for (uint16_t i = 0; i < gearRatios.size() && i < vGearRatios.size(); i++) {
                gearRatios[i] = vGearRatios[i];
            }

            // copy the gears
            if (engineReverse) {
                engineReverse->fullGears.set(gearRatios);
            } else if (engineGear) {
                engineReverse->gears.set(gearRatios);
            }
        }

    }


    /** Get the meassurement from the proc. */
    void getMeassurement(Meassurement* meas) {

        auto engineGear = dynamic_cast<rcEngine::EngineGear*>(engine);
        // auto engineBrake = dynamic_cast<rcEngine::EngineBrake*>(engine);
        auto engineReverse = dynamic_cast<rcEngine::EngineReverse*>(engine);

        switch (engine->state) {
        case rcEngine::EngineSimple::EngineState::OFF:
            meas->strState = "_";
            break;
        case rcEngine::EngineSimple::EngineState::CRANKING:
            meas->strState = "C";
            break;
        case rcEngine::EngineSimple::EngineState::ON:
            meas->strState = "O";
            break;
        default:
            meas->strState = "/";
        }

        meas->rpm = engine->getRPM();
        meas->power = engine->getPower(meas->rpm,
            static_cast<float>(meas->throttle) / RCSIGNAL_MAX);
        meas->energyEngine = engine->energyEngine.get();
        meas->energyVehicle = 0.0f;
        meas->gear = 0;
        meas->strGearState = "--";

        if (engineGear) {
            meas->gear = engineGear->gearCurrent;
            meas->energyVehicle = engineGear->energyVehicle.get();
            meas->speedVal = engineGear->energyVehicle.speed(engineGear->massVehicle);

            switch (engineGear->gearState) {
            case rcEngine::EngineGear::GearState::DECOUPLED:
                meas->strGearState = "D";
                break;
            case rcEngine::EngineGear::GearState::COUPLING:
                meas->strGearState = "c";
                break;
            case rcEngine::EngineGear::GearState::COUPLED:
                meas->strGearState = "C";
                break;
            case rcEngine::EngineGear::GearState::DECOUPLING:
                meas->strGearState = "d";
                break;
            default:
                meas->strGearState = "-";
                break;
            }
        }

        if (engineReverse) {
            switch (engineReverse->drivingState) {
            case rcEngine::EngineReverse::DrivingState::STOPPED_FWD:
                meas->strDrivingState = "S";
                break;
            case rcEngine::EngineReverse::DrivingState::STOPPED_BCK:
                meas->strDrivingState = "s";
                break;
            case rcEngine::EngineReverse::DrivingState::FORWARD:
                meas->strDrivingState = "F";
                break;
            case rcEngine::EngineReverse::DrivingState::BACKWARD:
                meas->strDrivingState = "R";
                break;
            default:
                meas->strDrivingState = "-";
                break;
            }

            if ((engineReverse->drivingState == rcEngine::EngineReverse::DrivingState::BACKWARD) ||
                (engineReverse->drivingState == rcEngine::EngineReverse::DrivingState::STOPPED_BCK)) {
                meas->speedVal = -meas->speedVal;
                meas->gear = -meas->gear;
            }
        }
    }


    /** Performs the engine test, filling out the meassurements. */
    void engineTest() {
        rcSignals::TimeMs STEP_TIME = 20u; // we need that step time to make a realistic simulation.

        // -- get some configs
        rcSignals::TimeMs simTimeMs;
        if (config.count("time")) {
            simTimeMs = config["time"].as<rcSignals::TimeMs>();
        }

        rcSignals::TimeMs reportingTimeMs;
        if (config.count("step")) {
            reportingTimeMs = config["step"].as<rcSignals::TimeMs>();
        }

        rcSignals::TimeMs breakTimeMs = simTimeMs;
        if (config.count("break")) {
            breakTimeMs = config["break"].as<rcSignals::TimeMs>();
        }

        // -- start
        inputDemo.start();
        engine->start();

        Signals signals;
        signals.reset();
        rcProc::StepInfo info = {
            .deltaMs = STEP_TIME,
            .signals = &signals,
            .intervals = {
                SamplesInterval{.first = nullptr, .last = nullptr},
                SamplesInterval{.first = nullptr, .last = nullptr}
            }
        };

        rcSignals::TimeMs nextReportTime = 0;
        for (rcSignals::TimeMs time = 0; time < simTimeMs; time += STEP_TIME) {

            signals.reset();
            inputDemo.step(info);

            bool doBreak = false;
            if (time >= breakTimeMs) {
                doBreak = true;
                breakTimeMs = numeric_limits<uint32_t>::max();
            }

            Meassurement meas;
            meas.time = time;
            meas.speedOrig = signals.get(SignalType::ST_SPEED, -9999);
            meas.throttleOrig = signals.get(SignalType::ST_THROTTLE, -9999);
            meas.brake = signals.get(SignalType::ST_BRAKE, -999);
            meas.power = 0.0f;
            meas.rpm = 0.0f;
            meas.speedVal = 0.0f;
            meas.energyEngine = 0.0f;
            meas.energyVehicle = 0.0f;
            meas.gear = 0;

            if (doBreak) {
                raise(SIGTRAP);
            }
            engine->step(info);

            meas.speed = signals.get(SignalType::ST_SPEED, -9999);
            meas.throttle = signals.get(SignalType::ST_THROTTLE, -9999);
            meas.brake = signals.get(SignalType::ST_BRAKE, -999);

            getMeassurement(&meas);

            if (time >= nextReportTime) {
                nextReportTime += reportingTimeMs;
                meassurements.push_back(meas);
            }
        }
    }

};


int main(int argc, char* argv[]) {


    // --- command line options
    po::options_description optSimulation("Simulation options");
    optSimulation.add_options()
        ("help,h", "produce help message")
        ("time",
            po::value<rcSignals::TimeMs>()->default_value(30000),
            "Total simulation time in milli seconds.")
        ("step,s",
            po::value<rcSignals::TimeMs>()->default_value(1000),
            "Total simulation time in milli seconds.")
        ("demo,d",
            po::value<std::string>()->default_value("truck"),
            "Demo type. One of \"truck\", \"train\" or \"car\".")
    ;

    po::options_description optEngine("Engine options");
    optEngine.add_options()
        ("engine,e",
            po::value<std::string>(),
            "Engine class. One of simple, gear, brake, reverse")
        ("type,t",
            po::value<std::string>(),
            "Engine type. One of steam, diesel, petrol, ...")
        ("massE,m",
            po::value<float>(),
            "Engine mass in kg.")
        ("massV,M",
            po::value<float>(),
            "Vehicle mass in kg.")
        ("power,p",
            po::value<float>(),
            "Engine max power in Watts.")
        ("powerBrake,P",
            po::value<float>(),
            "Brake max power in Watts.")
        ("airResistance",
            po::value<float>(),
            "Value for the air resistance")
        ("rpmMax,r",
            po::value<float>(),
            "Max RPM in rev per minute.")
        ("rpmIdle,R",
            po::value<float>(),
            "Idle RPM in rev per minute.")
        ("rpmShift,s",
            po::value<float>(),
            "RPM to keep when switching gears")
        ("timeCouple",
            po::value<rcSignals::TimeMs>(),
            "Time to engage the gear in ms")
        ("timeDecouple",
            po::value<rcSignals::TimeMs>(),
            "Time to disengage the gear in ms")
        ("timeDoubleDeclutch",
            po::value<rcSignals::TimeMs>(),
            "Double declutch (Zwischengas) time in ms")
        ("wheel,w",
            po::value<float>(),
            "Wheel diameter in Meters")
        ("gears,g",
            po::value< vector<float> >(),
            "List of gear ratios.")
    ;

    po::options_description optOther("Other options");
    optOther.add_options()
        ("break,b",
            po::value<rcSignals::TimeMs>(),
            "Trigger breakpoint (SIGINT) at time (in milli seconds)")
    ;

    po::options_description optAll;
    optAll.add(optSimulation).add(optEngine).add(optOther);

    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, optAll), vm);
        po::notify(vm);

    } catch (po::error const& e) {
        std::cerr << e.what() << std::endl;
        optAll.print(std::cerr);
        return 1;
    }

    if (vm.count("help")) {
        cout << optAll << "\n";
        return 1;
    }

    // --- run simulation
    EngineSimulator sim(vm);
    sim.engineTest();

    // --- output results
    cout << "  Time," <<
        "state,  " <<
        "speed,     " <<
        "throttle,  " <<
        "brake,    " <<
        "power,     " <<
        "RPM,      " <<
        "speed,          " <<
        "energyEngine, " <<
        "energyVehicle, " <<
        "gear " <<
        endl;
    for (const auto& mes : sim.meassurements) {

        cout << setprecision(1) << std::fixed <<
            setw(5) << (mes.time / 1000.0) << "s, " <<
            setw(1) << mes.strState << ", " <<

            setprecision(1) << std::fixed <<
            setw(5) << mes.speedOrig << "(" << setw(5) << mes.speed << "), " <<

            setprecision(0) << std::fixed <<
            setw(5) << mes.throttleOrig << "(" << setw(5) << mes.throttle << "), " <<

            setprecision(0) << std::fixed <<
            setw(4) << mes.brake << ", " <<

            setprecision(2) << std::fixed <<
            setw(8) << (mes.power / 746.0f) << "hp, " <<

            setprecision(1) << std::fixed <<
            setw(6) << mes.rpm << "r/min, " <<

            setprecision(1) << std::fixed <<
            setw(5) << mes.speedVal << "m/s (" <<
            setw(5) << (mes.speedVal * 3.6f) << "kph), " <<

            setprecision(0) << std::fixed <<
            setw(5) << (mes.energyEngine / 1000.0f) << "kJ, " <<

            setprecision(0) << std::fixed <<
            setw(5) << (mes.energyVehicle / 1000.0f) << "kJ, " <<

            setw(2) << mes.gear << " " <<

            mes.strGearState << " " <<
            mes.strDrivingState <<
            endl;
    }

    return 0;
}

