/**
 *  Implementation of the output via DAC
 *
 *  @file
*/

#include "output_audio.h"

#include "audio_ringbuffer.h"

#include "signals.h"
#include <cstdint>

#include <esp_check.h>

#include <esp_log.h>
#include <algorithm>
#include <cassert>

static const char* TAG = "OutAudio";

using namespace rcSignals;
using namespace rcAudio;

namespace rcOutput {


/** This is just a freerunning interrupt indicating that a DMA buffer
 *  has been processed and communicating the location of that buffer.
 */
static bool IRAM_ATTR  dac_on_convert_done_callback(dac_continuous_handle_t handle, const dac_event_data_t *event, void *user_data)
{
    QueueHandle_t queue = (QueueHandle_t)user_data;

    BaseType_t need_awoke;
    // When the queue is full, drop the oldest item
    if (xQueueIsQueueFullFromISR(queue)) {
        dac_event_data_t dummy;
        xQueueReceiveFromISR(queue, &dummy, &need_awoke);
    }
    // Send the event from callback
    xQueueSendFromISR(queue, event, &need_awoke);

    return need_awoke;
}

OutputAudio::OutputAudio() :
    handleDac(nullptr),
    handleQueue(nullptr) {

    // Create a queue for the DMA buffer locations
    handleQueue = xQueueCreate(rcAudio::AudioRingbuffer::NUM_BLOCKS, sizeof(dac_event_data_t));
    assert(handleQueue);
}

OutputAudio::~OutputAudio() {
    vQueueDelete(handleQueue);
}

/** Initializes this Audio output module.
 *
 *  We set up a continuous DAC with both channels.
 */
void OutputAudio::start()
{
    if (handleDac == nullptr) {
        ESP_LOGI(TAG, "Setup started");

        dac_continuous_config_t cont_cfg = {
            .chan_mask = DAC_CHANNEL_MASK_ALL, // use channel 0 and 1
            .desc_num = rcAudio::AudioRingbuffer::NUM_BLOCKS,
            .buf_size = rcAudio::AudioRingbuffer::BLOCK_SIZE * 4,  // 16 bit for two channels makes 4 bytes
            .freq_hz = SAMPLE_RATE,
            .offset = 0,
            .clk_src = DAC_DIGI_CLK_SRC_PLLD2,    // APLL might be used by others
            .chan_mode = DAC_CHANNEL_MODE_ALTER,  // data is alternate for channel 0 and 1
        };

        // Allocate continuous channels
        ESP_ERROR_CHECK(
            dac_continuous_new_channels(&cont_cfg, &handleDac));

        dac_event_callbacks_t cbs = {
            .on_convert_done = dac_on_convert_done_callback,
            .on_stop = NULL,
        };
        // The callback
        ESP_ERROR_CHECK(
            dac_continuous_register_event_callback(handleDac, &cbs, handleQueue));

        // enable
        ESP_ERROR_CHECK(
            dac_continuous_enable(handleDac));
        ESP_LOGI(TAG, "DAC initialized success, DAC DMA is ready");

        ESP_ERROR_CHECK(
            dac_continuous_start_async_writing(handleDac));

        ESP_LOGI(TAG, "Setup Done");
    }


    // fill some initial ringbuffer blocks to prevent clicking
    auto& ringbuffer = rcAudio::getRingbuffer();
    auto interval = ringbuffer.getEmptyBlocks();

    const uint16_t len = interval.last - interval.first;
    const uint16_t stepCnt = len / 127;
    uint16_t step = 0;
    rcProc::AudioSample sample = {-127, -127};

    for (auto pos = interval.first; pos < interval.last; pos++) {

        *pos = sample;

        step++;
        if (step > stepCnt) {
            step = 0;
            sample.channel1++;
            sample.channel2++;
        }
    }

    ringbuffer.setBlocksFull(interval);
}

/** De-initialize this Output module. */
void OutputAudio::stop() {

    if (handleDac != nullptr) {
        /*
        // clear the DMA buffer to prevent strange noises
        for (int i = 0; i < rcAudio::AudioRingbuffer::NUM_BLOCKS; i++) {
            xQueueReceive(handleQueue, &evt_data, portMAX_DELAY);
            memset(evt_data.buf, 0, evt_data.buf_size);
        }
        */

        ESP_ERROR_CHECK(
            dac_continuous_disable(handleDac));
        ESP_ERROR_CHECK(
            dac_continuous_del_channels(handleDac));
        handleDac = nullptr;
    }
}

void OutputAudio::step(const rcProc::StepInfo& info) {
    auto& ringbuffer = rcAudio::getRingbuffer();

    if (handleDac == nullptr) {
        return;
    }

    rcSignals::RcSignal masterVolume =
        info.signals->get(SignalType::ST_MASTER_VOLUME, rcSignals::RCSIGNAL_MAX);
    float fVolume = masterVolume / 1000.0f;

    // while we have full ringbuffer blocks and a DMA buffer to fill
    while (ringbuffer.getNumFull() > 0) {

        dac_event_data_t evt_data;
        if (xQueueReceive(handleQueue, &evt_data, 0)) {

            auto interval = ringbuffer.getFullBlocks();

            // we should get exactly one block now.
            // not zero and not more than one.
            assert(interval.last - interval.first == rcAudio::AudioRingbuffer::BLOCK_SIZE);
            // the DMA buffer is twice as long because of the 16 bit align
            assert(evt_data.buf_size == rcAudio::AudioRingbuffer::BLOCK_SIZE * 4);

            // convert the buffer to interleaved 8 bit.
            std::array<uint8_t, rcAudio::AudioRingbuffer::BLOCK_SIZE * 2> buffer;
            for (int32_t i = 0; i < rcAudio::AudioRingbuffer::BLOCK_SIZE; i++) {
                buffer[i * 2] =
                    std::clamp(
                        interval.first[i].channel1 * fVolume + 127,
                        0.0f, 255.0f);
                buffer[i * 2 + 1] =
                    std::clamp(
                        interval.first[i].channel2 * fVolume + 127,
                        0.0f, 255.0f);
            }

            // write to DMA
            ESP_ERROR_CHECK(
                dac_continuous_write_asynchronously(handleDac,
                    static_cast<uint8_t*>(evt_data.buf),
                    evt_data.buf_size,
                    buffer.begin(),
                    buffer.size(),
                    nullptr));
            ringbuffer.setBlocksEmpty(interval);

        } else {
            break; // no new free DMA buffer
        }
    }
}


} // namespace

