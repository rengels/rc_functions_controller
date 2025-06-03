/** RC functions controller for Arduino ESP32
 *
 *  Definitions for factory functions, creating the configuration
 *  from streams.
 *
 *  @file
*/

#include "proc_storage.h"
#include "simple_byte_stream.h"
#include "sample_storage_singleton.h"

#include "signals.h"

#include "input.h"
#include "input_demo.h"

#include "output.h"
#ifdef ARDUINO
#include "output_audio.h"
#include "output_led.h"
#endif

#include "proc.h"
#include "proc_auto.h"
#include "proc_cranking.h"
#include "proc_sequence.h"
#include "proc_fade.h"
#include "proc_group.h"
#include "proc_combine.h"
#include "proc_indicator.h"
#include "proc_random.h"

#include "engine_idle.h"
#include "engine_simple.h"
#include "engine_gear.h"
#include "engine_reverse.h"

#include "audio_simple.h"
#include "audio_noise.h"
#include "audio_loop.h"
#include "audio_engine.h"
#include "audio_steam.h"

#ifdef HAVE_NV
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

static const char* TAG = "ProcStorage";
#endif

#include <vector>
#include <array>
#include <span>

#define STORAGE_NAMESPACE "storage"

using namespace rcSignals;
using namespace rcProc;
using namespace rcAudio;

/** Creates a proc depending on the type.
 *
 *  Factory function for creating procs.
 *  Implemented in serialization.cpp
 */
Proc* createProc(ProcType type);

ProcStorage::ProcStorage() {

    createDefaultConfig();
}

ProcStorage::~ProcStorage() {
    clear();
}

void ProcStorage::createDefaultConfig() {
    clear();

    // input proc
    procs.push_back(new rcProc::ProcGroup(0, 1)); // input group
    procs.push_back(new rcInput::InputDemo(
        rcInput::InputDemo::DemoType::TRUCK));

    vehicleSteamTrain();
    // vehicleTruck();
    // vehicleCar();
    // vehicleShip();

    // output procs
    procs.push_back(new rcProc::ProcGroup(2, 2));
#ifdef ARDUINO
    procs.push_back(new rcOutput::OutputLed());
    procs.push_back(new rcOutput::OutputAudio());
#endif

}

void ProcStorage::vehicleSteamTrain() {
    auto& ss = SampleStorageSingleton::getInstance();


    procs.push_back(new rcProc::ProcGroup(1, 3)); // vehicle group

    // -- steam engine
    auto engine = new rcEngine::EngineReverse();
    engine->fullGears.set({0.0});
    engine->engineType = rcEngine::EngineSimple::EngineType::STEAM;
    engine->crankingTimeMs = 0;
    engine->massEngine = 2000;
    engine->massVehicle = 200000; // 100t locomotive + tender
    engine->maxPower = 1400000.0f;
    engine->rpmMax = 25.0f /* around 80kph */ / (M_PI * 1.6f) /* wheel diameter */ * 60.0f,
    engine->idleManager = rcEngine::Idle(0, 0, 0, 0, 0);
    engine->rpmShift = 0.0f;
    engine->gearDecouplingTime = 0.0f;
    engine->gearCouplingFactor = 0u;
    engine->gearDoubleDeclutch = false;
    engine->wheelDiameter = 1.6f;
    engine->brakePower = 2500000.0f;
    engine->resistance = 20000.0f;
    procs.push_back(engine);

    procs.push_back(new rcProc::ProcAuto()); // for parking brake

    // -- audio
    procs.push_back(new rcProc::ProcGroup(3, 8));

    procs.push_back(
        new AudioLoop(
            ss.getSampleData(rcSamples::AudioId({'T', 'S', 'w'})),
            6295,
            8157,
            SignalType::ST_HORN,
            {1.0f, 1.0f}));

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'T', 'A', 'B'})),
            SignalType::ST_PARKING_BRAKE,
            {0.4f, 0.4f}));

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'T', 'C', 'O'})),
            SignalType::ST_TRAILER_SWITCH,
            {0.2f, 0.2f}));

    procs.push_back(
        new ProcFade(5, 5, {SignalType::ST_IGNITION, SignalType::ST_NONE}
            ));

    procs.push_back(
        new AudioNoise(
            SignalType::ST_IGNITION,
            rcAudio::AudioNoise::NoiseType::PINK,
            {0.01f, 0.01f}));

    procs.push_back(
        new AudioSteam(
            2,
            0.0f,
            0.001f,
            0.0005f,
            {0.15f, 0.15f}));
    procs.push_back(
        new AudioSteam(
            5,
            0.1f,
            0.002f,
            0.0005f,
            {0.2f, 0.2f}));

    procs.push_back(
        new AudioLoop(
            ss.getSampleData(rcSamples::AudioId({'T', 'B', 'R'})),
            106459,
            120854,
            SignalType::ST_BRAKE,
            {0.1f, 0.1f}));
}

void ProcStorage::vehicleShip() {
    auto& ss = SampleStorageSingleton::getInstance();

    procs.push_back(new rcProc::ProcGroup(1, 3)); // vehicle group

    // the numbers below are a little bit unrealistic, but
    // it wouldn't be a funny boat otherwise
    auto engine = new rcEngine::EngineReverse();
    engine->engineType = rcEngine::EngineSimple::EngineType::PETROL;
    engine->crankingTimeMs = 1000.0f;
    engine->massEngine = 80.0f;
    engine->massVehicle = 500.0f;  // mass of the screw
    engine->brakePower = 40000.0f;
    engine->maxPower = 4000.0f;
    engine->rpmMax = 900.0f;
    engine->idleManager = rcEngine::Idle(400, 450, 2, 7000, 10);
    engine->rpmShift = 650.0f;
    engine->gearDecouplingTime = 400.0f;
    engine->gearCouplingFactor = 100u;
    engine->gearDoubleDeclutch = false;
    engine->fullGears.set({-6.0f, 6.0f, 0.0f});
    engine->wheelDiameter = 1.0f;
    engine->resistance = 2000.0f;
    engine->airResistance = 4.0f;
    procs.push_back(engine);

    // at this point ST_SPEED is the speed of the screw
    // and not the one of the boat.
    procs.push_back(new ProcFade(2, 2, {SignalType::ST_SPEED, SignalType::ST_NONE}));

    procs.push_back(new rcProc::ProcGroup(3, 5)); // audio group

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'S', 'B', 'L'})),
            SignalType::ST_SIREN,
            {1.0f, 1.0f}));

    procs.push_back(
        new AudioLoop(
            ss.getSampleData(rcSamples::AudioId({'S', 'H', 'O'})),
            10383,
            19652,
            SignalType::ST_HORN,
            {1.0f, 1.0f}));

    procs.push_back(
        new AudioEngine(
            {ss.getSampleData(rcSamples::AudioId({'O', 'S', '2'})),
             ss.getSampleData(rcSamples::AudioId({'O', 'S', '3'})),
             ss.getSampleData(rcSamples::AudioId({'O', 'S', '4'})),
             ss.getSampleData(rcSamples::AudioId({'O', 'S', '5'})),
             ss.getSampleData(rcSamples::AudioId({'O', 's', 'i'})),
            },
            {100, 800, 300, 800, RCSIGNAL_MAX},
            {0.5f, 0.5f}));

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'O', 'S', '1'})),
            SignalType::ST_IGNITION,
            {0.5f, 0.5f}));

    // wave sounds
    procs.push_back(
        new AudioNoise(
            SignalType::ST_SPEED,
            rcAudio::AudioNoise::NoiseType::WHITE,
            {0.1f, 0.1f}));
}

void ProcStorage::vehicleTruck() {
    auto& ss = SampleStorageSingleton::getInstance();

    procs.push_back(new rcProc::ProcGroup(1, 4)); // vehicle group

    auto engine = new rcEngine::EngineReverse();
    engine->engineType = rcEngine::EngineSimple::EngineType::DIESEL;
    engine->crankingTimeMs = 1000;  // length of cranking sample minus fadeout
    engine->massEngine = 700.0f;
    engine->massVehicle = 10000.0f;
    engine->brakePower = 3700000.0f;
    engine->maxPower = 370000.0f;
    engine->idleManager = rcEngine::Idle(1100, 800, 10, 2000, 10);
    engine->rpmMax = 5500.0f;
    engine->rpmShift = 1100.0f;
    engine->gearDecouplingTime = 200.0f;
    engine->gearCouplingFactor = 100u;
    engine->gearDoubleDeclutch = true;
    engine->fullGears.set({5.4f, 3.6f, 2.5f, 1.8f, 1.3f, 1.0f, -5.4f, -3.6f, 0.0f});
    engine->wheelDiameter = 1.0f;
    engine->airResistance = 2.0f;
    procs.push_back(engine);

    procs.push_back(new rcProc::ProcAuto());
    procs.push_back(new rcProc::ProcIndicator());

    procs.push_back(new rcProc::ProcGroup(3, 9)); // audio group

    procs.push_back(
        new AudioEngine(
            {ss.getSampleData(rcSamples::AudioId({'T', 'D', '1'})),
             ss.getSampleData(rcSamples::AudioId({'T', 'D', '2'})),
             ss.getSampleData(rcSamples::AudioId({'T', 'D', '3'})),
             ss.getSampleData(rcSamples::AudioId({'T', 'D', '4'})),
             ss.getSampleData(rcSamples::AudioId({'O', 's', 'i'}))
            },
            {0, 100, 100, 500, 0},
            {0.5f, 0.5f}));

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'T', 'D', 'S'})),
            SignalType::ST_IGNITION,
            {0.5f, 0.5f}));

    procs.push_back(
        new ProcCombine(
            SignalType::ST_INDICATOR_RIGHT, SignalType::ST_INDICATOR_LEFT,
            SignalType::ST_AUX1, SignalType::ST_NONE,
            rcProc::ProcCombine::Function::F_OR));

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'C', 'I', 'N'})),
            SignalType::ST_AUX1));

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'T', 'A', 'B'})),
            SignalType::ST_PARKING_BRAKE,
            {0.5f, 0.5f}));

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'T', 'C', 'O'})),
            SignalType::ST_TRAILER_SWITCH,
            {0.5f, 0.5f}));

    procs.push_back(
        new AudioLoop(
            ss.getSampleData(rcSamples::AudioId({'T', 'R', 'E'})),
            0,
            29198,
            SignalType::ST_REVERSING,
            {0.2f, 0.2f}));

    procs.push_back(
        new AudioLoop(
            ss.getSampleData(rcSamples::AudioId({'S', 'i', '1'})),
            0,
            14244,
            SignalType::ST_SIREN,
            {1.0f, 1.0f}));

    procs.push_back(
        new AudioLoop(
            ss.getSampleData(rcSamples::AudioId({'T', 'H', 'O'})),
            1504,
            2476,
            SignalType::ST_HORN,
            {0.9f, 0.9f}));
}

void ProcStorage::vehicleCar() {
    auto& ss = SampleStorageSingleton::getInstance();

    procs.push_back(new rcProc::ProcGroup(1, 6)); // vehicle group

    auto engine = new rcEngine::EngineReverse();
    engine->engineType = rcEngine::EngineSimple::EngineType::PETROL;
    engine->crankingTimeMs = 778;  // length of cranking sample minus fadeout
    engine->massEngine = 100.0f;
    engine->massVehicle = 800.0f;
    engine->brakePower = 160000.0f;
    engine->maxPower = 32000.0f;
    engine->idleManager = rcEngine::Idle(900, 800, 10, 2000, 30);
    engine->rpmMax = 3300.0f;
    engine->rpmShift = 1000.0f;
    engine->gearDecouplingTime = 300.0f;
    engine->gearCouplingFactor = 80u;
    engine->gearDoubleDeclutch = false;
    engine->fullGears.set({-3.8f, 3.8f, 2.06f, 1.26f, 0.0f});
    engine->wheelDiameter = 0.5f;
    engine->airResistance = 1.0f;
    procs.push_back(engine);

    procs.push_back(new rcProc::ProcAuto());
    procs.push_back(new rcProc::ProcIndicator());

    // light bulbs fading
    procs.push_back(new ProcFade());
    procs.push_back(new ProcCranking());

    procs.push_back(new rcProc::ProcGroup(3, 6)); // audio group

    procs.push_back(
        new AudioEngine(
            {ss.getSampleData(rcSamples::AudioId({'C', 'V', '2'})),
             ss.getSampleData(rcSamples::AudioId({'C', 'V', '3'})),
             ss.getSampleData(rcSamples::AudioId({'C', 'V', '4'})),
             ss.getSampleData(rcSamples::AudioId({'C', 'V', '5'})),
             ss.getSampleData(rcSamples::AudioId({'O', 's', 'i'}))
            },
            {150, 900, 150, 900, 0},
            {0.5f, 0.5f}));

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'C', 'V', '1'})),
            SignalType::ST_IGNITION,
            {0.5f, 0.5f}));

    procs.push_back(
        new ProcCombine(
            SignalType::ST_INDICATOR_RIGHT, SignalType::ST_INDICATOR_LEFT,
            SignalType::ST_AUX1, SignalType::ST_NONE,
            rcProc::ProcCombine::Function::F_OR));

    procs.push_back(
        new AudioSimple(
            ss.getSampleData(rcSamples::AudioId({'C', 'I', 'N'})),
            SignalType::ST_AUX1));

    procs.push_back(
        new AudioLoop(
            ss.getSampleData(rcSamples::AudioId({'S', 'i', '1'})),
            0,
            14244,
            SignalType::ST_SIREN,
            {1.0f, 1.0f}));

    procs.push_back(
        new AudioLoop(
            ss.getSampleData(rcSamples::AudioId({'C', 'H', 'O'})),
            902,
            1505,
            SignalType::ST_HORN,
            {0.5f, 0.5f}));

}

void ProcStorage::clear() {
    // destroy the old procs
    for (const auto& proc : procs) {
        delete proc;  // no need to call proc->stop(). The destructor does that automatically
    }
    procs.resize(0);
}

void ProcStorage::start() {
    for (const auto& proc : procs) {
        proc->start();
    }
}

void ProcStorage::stop() {
    for (const auto& proc : procs) {
        proc->stop();
    }
}

void ProcStorage::step(const StepInfo& info) {

    for (Proc* proc : procs) {
        (*(info.signals))[SignalType::ST_NONE] = RCSIGNAL_NEUTRAL; // ensure that this signal stays neutral.
        proc->step(info);
    }
}

void ProcStorage::loadFromNvm() {
#ifdef HAVE_NV
    nvs_handle_t nvsHandle;

    // Open
    ESP_ERROR_CHECK(
        nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle));

    size_t bufferSize = 0;

    esp_err_t ret;
    ret = nvs_get_blob(nvsHandle, "config", NULL, &bufferSize);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No NVM found");

    } else if (bufferSize == 0) {
        ESP_LOGI(TAG, "No config NVM found");

    } else {
        uint8_t* buffer = new uint8_t[bufferSize];
        ESP_ERROR_CHECK(
            nvs_get_blob(nvsHandle, "config", buffer, &bufferSize));
        std::span<const uint8_t> sp(buffer, bufferSize);
        SimpleInStream stream(sp);
        deserialize(stream);

        delete[] buffer;

        ESP_LOGI(TAG, "Loaded config from NVM");
    }

    nvs_close(nvsHandle);
#endif
}

void ProcStorage::saveToNvm() const {
#ifdef HAVE_NV
    nvs_handle_t nvsHandle;

    // Open
    ESP_ERROR_CHECK(
        nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle));

    SimpleOutStream stream;
    serialize(stream);

    if (stream.fail()) {
        ESP_LOGW(TAG, "Ran out of buffer writing NVM");
    } else {
        ESP_ERROR_CHECK(
            nvs_set_blob(nvsHandle, "config",
                stream.buffer().data(), stream.buffer().size()));

        ESP_LOGI(TAG, "Wrote config to NVM");
    }

    free(stream.buffer().data());
    nvs_close(nvsHandle);
#endif
}

void ProcStorage::serialize(SimpleOutStream& out) const {

    // write header
    out.writeUint8('R');
    out.writeUint8('C');
    out.writeUint8(1U);  // binary format version

    // serialize procs
    out.write<uint8_t>(procs.size());
    for (const auto& proc : procs) {
        serializeProc(out, *proc);
    }
}

bool ProcStorage::deserialize(SimpleInStream& in) {

    // check header
    auto b1 = in.read<uint8_t>();
    auto b2 = in.read<uint8_t>();
    auto b3 = in.read<uint8_t>();
    if (b1 != 'R' || b2 != 'C' || b3 != 1U) {
#ifdef HAVE_NV
        ESP_LOGW(TAG, "Config header incorrect.");
#endif
        return false;
    }

    clear();

    // insert procs
    const uint8_t count = in.read<uint8_t>();
    for (uint8_t i = 0; i < count; i++) {
        auto proc = deserializeProc(in);
#ifdef HAVE_NV
        ESP_LOGI(TAG, "Desr Proc %d succ? %d fail? %d.",
            static_cast<int>(i),
            static_cast<int>(proc != nullptr),
            static_cast<int>(in.fail()));
#endif
        if (proc != nullptr) {
            procs.push_back(proc);
        }

        if (in.fail()) {
            break;
        }
    }

#ifdef HAVE_NV
    ESP_LOGI(TAG, "Read %d config blocks out of %d.",
        procs.size(), static_cast<int>(count));
#endif

    // if no procs, add at least the default procs.
    if (procs.size() == 0) {
        createDefaultConfig();
    }
    return true;
};

