/** RC functions controller for Arduino ESP32
 *
 *  @file
 *
*/

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/idf_additions.h> // for xTaskCreatePinnedToCore
#include <nvs_flash.h>

#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_heap_task_info.h> // for task info

#include <driver/gpio.h>  // for gpio_dump_all_io_configuration

#include <span>

static const char* TAG = "RcVehicle";

#include "proc_storage.h"

#include "audio_ringbuffer.h"
#include "signals.h"
#include "simple_byte_stream.h"
#include "sample_storage_singleton.h"
#include "bluetooth.h"

#include "esp_timer.h"  // for exact time
#include "rtc_wdt.h"  // for watchdog timer

extern "C" {

void mainTask(void *pvParameters);
void statusTask(void *pvParameters);

using namespace rcSignals;

/** Main signal buffer.
 *
 *  We assume atomar writes, so no protection with semaphore or such.
 */
static Signals signals;

/** Bluetooth signal buffer.
 *
 *  This struct contains the signals send via bluetooth.
 *  It will override input signals.
 */
static Signals signalsBt;

/** Main configuration containing all the procs */
ProcStorage storage = ProcStorage();

int64_t minTaskTime = 999;
int64_t maxTaskTime = 0;
int64_t lastTaskTime = 0;

void updateBluetoothAudioList();

/** This function interfaces between the signals used in the main
 *  task and the signals send and received via bluetooth.
 *
 *  - writes the signals output queue
 *  - checks the signals input queue for new message and reads it.
 */
void updateBluetoothSignals() {
    // send new signals out
    {
        // Two full signal buffers for output.
        // So we can swap those two around without reallocs
        static std::array<SimpleOutStream, 2> signalOutStreams;
        static uint8_t bufferIndex = 0u;

        signalOutStreams[bufferIndex].seekg(0);
        signalOutStreams[bufferIndex] << signals.signals;

        QueueByteBuffer newOutBuffer(
            signalOutStreams[bufferIndex].buffer().data(),
            signalOutStreams[bufferIndex].tellg());
        xQueueOverwrite(queueOutSignals,
            &(newOutBuffer));

        bufferIndex = (bufferIndex + 1u) % signalOutStreams.size();
    }

    // receive signals
    QueueByteBuffer inBuffer;
    bool signalsReceived = xQueueReceive(
        queueInSignals,
        &inBuffer,
        static_cast<TickType_t>(0));
    if (signalsReceived) {
        ESP_LOGI(TAG, "Received new signals");  // might delay audio long enough to cause issues
        std::span<const uint8_t> sp(
            static_cast<const uint8_t*>(inBuffer.data),
            inBuffer.len);
        SimpleInStream in(sp);
        in >> signalsBt.signals;

        free(inBuffer.data);
    }
}

/** This function interfaces between the Storage used in the main
 *  task and the config send and received via bluetooth.
 *
 *  - writes the config output queue
 *  - checks the config input queue for new message and reads it.
 */
void updateBluetoothConfig() {
    // the last buffer SimpleOutStream allocated for us.
    // we want to free it once we created a new one.
    static QueueByteBuffer lastOutBuffer(static_cast<uint8_t*>(nullptr), 0);

    // receive config
    QueueByteBuffer inBuffer;
    bool configReceived = xQueueReceive(
        queueInConfig,
        &inBuffer,
        static_cast<TickType_t>(0));
    if (configReceived) {
        ESP_LOGI(TAG, "Received new config");

        std::span<const uint8_t> sp(
            static_cast<const uint8_t*>(inBuffer.data),
            inBuffer.len);
        SimpleInStream in(sp);

        storage.stop();
        storage.deserialize(in);
        storage.saveToNvm();
        storage.start();

        free(inBuffer.data);

        // update the audio list:
        // - a reset might have removed all samples
        // - after uploading samples the config is also updated,
        //    so this is a good time to refresh the list
        updateBluetoothAudioList();
    }

    // send initial config out or update the out queue
    if (configReceived ||
        (uxQueueMessagesWaiting(queueOutConfig) == 0)) {

        SimpleOutStream out;
        storage.serialize(out);

        QueueByteBuffer newOutBuffer(out.buffer().data(), out.tellg());
        xQueueOverwrite(queueOutConfig, &newOutBuffer);
        // TODO: we might be too quick deallocating the buffer
        free(lastOutBuffer.data);
        lastOutBuffer = newOutBuffer;
    }
}

/** This function interfaces between the SampleStorage used in the main
 *  task and the audio list send via bluetooth.
 *
 *  This function is called whenever we suspect that the audio list
 *  changes are done.
 *
 *  - updates the audio out queue.
 */
void updateBluetoothAudioList() {
    // the last buffer SimpleOutStream allocated for us.
    // we want to free it once we created a new one.
    static QueueByteBuffer lastOutBuffer(static_cast<uint8_t*>(nullptr), 0);

    auto& ss = SampleStorageSingleton::getInstance();

    SimpleOutStream out;
    ss.serializeList(out);

    QueueByteBuffer newOutBuffer(out.buffer().data(), out.tellg());
    xQueueOverwrite(queueOutAudioList, &newOutBuffer);
    // TODO: we might be too quick deallocating the buffer
    // use shared ptr or atomic ref counter or something like that.
    // isn't there a dynamic memory queue
    free(lastOutBuffer.data);
    lastOutBuffer = newOutBuffer;
}

/** This function interfaces between the SampleStorage used in the main
 *  task and the audio sample received via bluetooth.
 *
 *  - checks the audio input queue for new message and reads it.
 */
void updateBluetoothAudio() {

    auto& ss = SampleStorageSingleton::getInstance();

    // receive Audio
    QueueByteBuffer inBuffer;
    bool audioReceived = xQueueReceive(
        queueInAudio,
        &inBuffer,
        static_cast<TickType_t>(0));
    if (audioReceived) {
        ESP_LOGI(TAG, "Received new audio");

        // we should stop audio playback here in case of a reset of the dynamic samples
        std::span<const uint8_t> sp(
            static_cast<const uint8_t*>(inBuffer.data),
            inBuffer.len);
        SimpleInStream in(sp);
        ss.executeCommand(in);

        free(inBuffer.data);
    }

}

#define MAX_TASK_NUM 20                         // Max number of per tasks info that it can store
#define MAX_BLOCK_NUM 20                        // Max number of per block info that it can store

static size_t s_prepopulated_num = 0;
static heap_task_totals_t s_totals_arr[MAX_TASK_NUM];
static heap_task_block_t s_block_arr[MAX_BLOCK_NUM];

static void esp_dump_per_task_heap_info()
{
    heap_task_info_params_t heap_info;
    heap_info.caps[0] = MALLOC_CAP_8BIT;        // Gets heap with CAP_8BIT capabilities
    heap_info.mask[0] = MALLOC_CAP_8BIT;
    heap_info.caps[1] = MALLOC_CAP_32BIT;       // Gets heap info with CAP_32BIT capabilities
    heap_info.mask[1] = MALLOC_CAP_32BIT;
    heap_info.tasks = NULL;                     // Passing NULL captures heap info for all tasks
    heap_info.num_tasks = 0;
    heap_info.totals = s_totals_arr;            // Gets task wise allocation details
    heap_info.num_totals = &s_prepopulated_num;
    heap_info.max_totals = MAX_TASK_NUM;        // Maximum length of "s_totals_arr"
    heap_info.blocks = s_block_arr;             // Gets block wise allocation details. For each block, gets owner task, address and size
    heap_info.max_blocks = MAX_BLOCK_NUM;       // Maximum length of "s_block_arr"

    heap_caps_get_per_task_info(&heap_info);

    for (int i = 0 ; i < *heap_info.num_totals; i++) {
        printf("Task: %s -> CAP_8BIT: %d CAP_32BIT: %d\n",
                heap_info.totals[i].task ? pcTaskGetName(heap_info.totals[i].task) : "Pre-Scheduler allocs" ,
                heap_info.totals[i].size[0],    // Heap size with CAP_8BIT capabilities
                heap_info.totals[i].size[1]);   // Heap size with CAP32_BIT capabilities
    }

    printf("\n\n");
}

void app_main(void) {

    ESP_LOGI(TAG, "Setup started");

    // -- setup flash
    ESP_LOGI(TAG, "Setup flash");
    // necessary for BT and configuration data
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_LOGW(TAG, "Erasing flash");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // -- setup pipeline
    ESP_LOGI(TAG, "Setup pipeline");

    storage.loadFromNvm();
    storage.start();

    // -- setup bluetooth
    ESP_LOGI(TAG, "Setup bluetooth");
    signalsBt.reset();
    btStart();

    updateBluetoothConfig();
    updateBluetoothAudioList();

    // -- setup watchdog
    // TODO: watchdogs

    // -- setup tasks
    // we have the status task the main task (running the main loop)
    // and the bluetooth stuff which also has a task

    ESP_LOGI(TAG, "Setup tasks");
    TaskHandle_t Task2;
    xTaskCreatePinnedToCore(
        statusTask,   // Task function
        "statusTask", // name of task
        10240,       // Stack size of task (1024)
        nullptr,    // parameter of the task
        1,          // priority of the task (1 = low, 3 = medium, 5 = highest)
        &Task2,     // Task handle to keep track of created task
        0);         // pin task to core 0

    ESP_LOGI(TAG, "Setup finished");

    mainTask(nullptr); // will not return

    btStop();
}

/** Task function for the main task
 *
 *  This task is scheduled every 20ms and
 *  does:
 *
 *  - call input modules to get new input signals
 *  - call mapper to map the input to processed signals
 *  - call processors and effects to further determine signals
 *  - call output modules to call their step functions.
 *
 */
void mainTask(void *pvParameters) {

    auto& ringbuffer = rcAudio::getRingbuffer();
    TickType_t lastWakeTime;
    const TickType_t frequencyTick = 20U / portTICK_PERIOD_MS; // wake up ever 20 ms

    lastWakeTime = xTaskGetTickCount();
    for (;;) {
        xTaskDelayUntil(&lastWakeTime, frequencyTick);
        int64_t timeStart = esp_timer_get_time();

        // -- prepare StepInfo
        // signals.reset();
        signals = signalsBt;

        rcProc::StepInfo info = {
            .deltaMs = 20U, // TODO
            .signals = &signals,
            .intervals = {ringbuffer.getEmptyBlocks(),
                ringbuffer.getEmptyBlocks()}
        };

        // -- call all the steps
        storage.step(info);

        ringbuffer.setBlocksFull(info.intervals[0]);
        ringbuffer.setBlocksFull(info.intervals[1]);

        updateBluetoothSignals();
        updateBluetoothConfig();
        updateBluetoothAudio();
        // rtc_wdt_feed();

        int64_t timeEnd = esp_timer_get_time();
        lastTaskTime = timeEnd - timeStart;
        if (lastTaskTime > maxTaskTime) {
            maxTaskTime = lastTaskTime;
        }
        if (lastTaskTime < minTaskTime) {
            minTaskTime = lastTaskTime;
        }
    }
}

/** Task function for the status task.
 *
 *  Runs on core 0
 *  This task is scheduled every 5000ms and
 *  prints out status information via the serial port.
 */
void statusTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(5000U / portTICK_PERIOD_MS);
        /* for debugging heap usage
        esp_dump_per_task_heap_info();
        */

        printf("Main task us: min: %lld last: %lld max: %lld\n",
               minTaskTime, lastTaskTime, maxTaskTime);
        minTaskTime = lastTaskTime;
        maxTaskTime = lastTaskTime;

        /* for debugging of outputs
        vTaskDelay(5000U / portTICK_PERIOD_MS);
        gpio_dump_io_configuration(stdout, SOC_GPIO_VALID_GPIO_MASK);
        */
    }

}

} // extern "C"
