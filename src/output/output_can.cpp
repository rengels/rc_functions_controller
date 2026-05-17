/**
 *  Implementation of the output/input via CAN
 *
 *  IMPORTANT: The twai interface seems to be in a flux right now.
 *  I am using ESP-idf 5.5 right now!
 *
 *  @file
*/

#include "output_can.h"
#include "../input/input_can.h"
#include "signals.h"

#include <cstdint>
#include <algorithm>

#ifdef HAVE_NV
#include <freertos/FreeRTOS.h>
#include <esp_timer.h>  // for exact time
#include <esp_twai.h>
#include <esp_twai_onchip.h>
#include <esp_log.h>
#include <esp_err.h>
#endif

static const char* TAG = "OutCan";

using namespace rcSignals;
using namespace rcInput;

namespace rcOutput {

#ifdef HAVE_NV
QueueHandle_t canRxQueue = nullptr;
twai_node_handle_t canDriverHandle = nullptr;
#endif

CanId CanDriverWrapper::filterIds = ID_UNSET;
CanId CanDriverWrapper::filterMask = 0x7FFu;
std::atomic_int8_t CanDriverWrapper::driverRefCount(0);
std::vector<CanMessage> CanDriverWrapper::receivedMessages;


static bool twai_listener_rx_callback(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    BaseType_t woken = pdFALSE;

    uint8_t recv_buff[8];
    twai_frame_t frame = {
        .header = {
           .id = 0,
           .dlc = 0,
           .ide = false,
           .rtr = false,  // remote
           .fdf = false,
           .brs = false,  // bit rate shift
           .esi = false,  // side error indicator
           .trigger_time = 0u,
        },
        .buffer = recv_buff,
        .buffer_len = sizeof(recv_buff),
    };

    if (ESP_OK == twai_node_receive_from_isr(handle, &frame)) {

        CanMessage msg = {
            .id = static_cast<CanId>(frame.header.id),
            .data = {frame.buffer[0], frame.buffer[1], frame.buffer[2],
              frame.buffer[3], frame.buffer[4], frame.buffer[5],
              frame.buffer[6], frame.buffer[7]},
            .len = static_cast<uint8_t>(frame.header.dlc),
            .recTime = 0};  // leave the current time 0 for now. We do this outside of the ISR

        xQueueSendToBackFromISR(canRxQueue,
                                &msg,
                                &woken );
    }
    return (woken == pdTRUE);
}


CanDriverWrapper::CanDriverWrapper() {
    uint8_t old = driverRefCount.fetch_add(1);

    // check if we have to initialize the driver
    if (old == 0) {
        // create our input queue if not already done so
        if (canRxQueue == 0) {
            canRxQueue = xQueueCreate(OutputCan::CAN_QUEUE_LEN, sizeof(CanMessage));
        }

        startDriver();
    }
}


CanDriverWrapper::~CanDriverWrapper() {
    uint8_t old = driverRefCount.fetch_sub(1);

    if (old == 1) {
        stopDriver();
    }
}


void CanDriverWrapper::startDriver() {

    if (!canDriverHandle) {
        twai_onchip_node_config_t node_config = {
            .io_cfg = {
                .tx = TX_GPIO_NUM,
                .rx = RX_GPIO_NUM,
                .quanta_clk_out = static_cast<gpio_num_t>(-1),
                .bus_off_indicator = static_cast<gpio_num_t>(-1),
            },
            .clk_src = static_cast<twai_clock_source_t>(0),
            .bit_timing = {
                .bitrate = TWAI_BITRATE,
                .sp_permill = 0,
                .ssp_permill = 0,
            },
            .data_timing = {
                .bitrate = TWAI_BITRATE,
                .sp_permill = 0,
                .ssp_permill = 0,
            },
            .fail_retry_cnt = -1,  // retransmit forever
            .tx_queue_depth = OutputCan::CAN_QUEUE_LEN,
            .intr_priority = 1,
            .flags = {
                .enable_self_test = false,
                .enable_loopback = false,
                .enable_listen_only = false,
                .no_receive_rtr = false,
            }
        };

        // Create a new TWAI controller driver instance
        ESP_ERROR_CHECK(
          twai_new_node_onchip(&node_config, &canDriverHandle));

        // Register callbacks
        twai_event_callbacks_t callbacks = {
            .on_tx_done = nullptr,
            .on_rx_done = twai_listener_rx_callback,
            .on_state_change = nullptr,
            .on_error = nullptr,
        };
        ESP_ERROR_CHECK(
            twai_node_register_event_callbacks(canDriverHandle, &callbacks, this));

        // for now, filter everything. This needs to be done when node is stopped
        filterIds = ID_UNSET;
        filterMask = 0x7FFu;
        twai_mask_filter_config_t mfilter_cfg = {
            .id = (filterIds & 0x7FFu),
            .mask = (filterMask & 0x7FFu),
            .is_ext = false,  // Accept only standard IDs, not extended IDs
            .no_classic = false,
            .no_fd = false,
            .dual_filter = false,
        };
        ESP_ERROR_CHECK(
              twai_node_config_mask_filter(canDriverHandle, 0, &mfilter_cfg));

        // Start the TWAI controller
        ESP_ERROR_CHECK(
            twai_node_enable(canDriverHandle));

    }
}

void CanDriverWrapper::stopDriver() {
    if (canDriverHandle) {
        int res = twai_node_disable(canDriverHandle);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Error when stopping CAN driver");
        }
        res = twai_node_delete(canDriverHandle);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Error when uninstalling CAN driver");
        }
        canDriverHandle = nullptr;

        filterIds = ID_UNSET;
        filterMask = 0x7FFu;
    }
}


void CanDriverWrapper::reconfigureFilter() {
    if (canDriverHandle) {
        int res = twai_node_disable(canDriverHandle);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Error when stopping CAN for reconfiguring.");
        } else {
            twai_mask_filter_config_t mfilter_cfg = {
                .id = (filterIds & 0x7FFu),
                .mask = (filterMask & 0x7FFu),
                .is_ext = false,  // Accept only standard IDs, not extended IDs
                .no_classic = false,
                .no_fd = false,
                .dual_filter = false,
            };
            res = twai_node_config_mask_filter(canDriverHandle, 0, &mfilter_cfg); // Configure on filter 0
            if (res != ESP_OK) {
                ESP_LOGE(TAG, "Error when configuring CAN filter mask.");
            }

            res = twai_node_enable(canDriverHandle);
            if (res != ESP_OK) {
                ESP_LOGE(TAG, "Error when restarting CAN.");
            }
        }
    }
}


void CanDriverWrapper::start(const rcProc::Proc* proc) {
    // check if we have to update the filter IDs
    auto procIn = dynamic_cast<const rcInput::InputCan*>(proc);
    if (procIn != nullptr) {
        if (filterIds == ID_UNSET) {
            filterIds = procIn->canId;
            filterMask = 0u;
        } else {
            CanId differences = (filterIds ^ procIn->canId);
            filterMask &= ~differences;
        }
        reconfigureFilter();
    }
}


void CanDriverWrapper::stop(const rcProc::Proc* proc) {
    // nothing special here
}


void CanDriverWrapper::removeOldMessages() {

    int64_t currentTime = esp_timer_get_time() / 1000;  // in us
    receivedMessages.erase(
      std::remove_if(receivedMessages.begin(),
                     receivedMessages.end(),
                     [currentTime](CanMessage x) {
                         return (x.recTime + ALIVE_TIME < currentTime) ||
                           (x.recTime > currentTime);
                     }),
               receivedMessages.end());
}


void CanDriverWrapper::readMessages() {
    int64_t currentTime = esp_timer_get_time() / 1000;

    // get the messages from the receive queue and put it into the vector
    CanMessage msg;
    while (xQueueReceive(canRxQueue, &msg, static_cast<TickType_t>(0))) {
        msg.recTime = currentTime;
        receivedMessages.push_back(msg);
    }
}


void CanDriverWrapper::sendMessage(const rcOutput::OutputCan* procOut, const rcProc::StepInfo& info) {

    // put our signals in the data
    uint8_t send_buf[OutputCan::CAN_NUM * 3] = {0};
    uint8_t len = 0;
    for (uint8_t i = 0; i < OutputCan::CAN_NUM && len + 3 < 8; i++) {
        if (procOut->types[i] != rcSignals::SignalType::ST_NONE) {
            const int16_t signal = (*(info.signals))[procOut->types[i]];
            send_buf[len++] = rcSignals::signalTypeToId(procOut->types[i]);
            send_buf[len++] = ((signal >> 8) & 0xFF);
            send_buf[len++] = ((signal >> 0) & 0xFF);
        }
    }

    twai_frame_t msg = {
        .header = {
            .id = procOut->canId,
            .dlc = len,
            .ide = false,
            .rtr = false,  // remote
            .fdf = false,
            .brs = false,  // bit rate shift
            .esi = false,  // side error indicator
            .trigger_time = 0u,
        },
        .buffer = send_buf,  // Pointer to data to transmit
        .buffer_len = len,  // Length of data to transmit
    };
    int res = twai_node_transmit(canDriverHandle, &msg, 0);  // Timeout = 0: returns immediately if queue is full
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Error when sending CAN message.");
    }
}


void CanDriverWrapper::step(const rcProc::Proc* proc, const rcProc::StepInfo& info) {

    auto procOut = dynamic_cast<const rcOutput::OutputCan*>(proc);
    auto procIn = dynamic_cast<const rcInput::InputCan*>(proc);

    if (procOut != nullptr) {

        // before trying to put on a new message, check the node state
        twai_node_status_t canStatus;
        twai_node_record_t canStatistics;
        int res = twai_node_get_info(canDriverHandle, &canStatus, &canStatistics);

        if ((res == ESP_OK) &&
            (canStatus.state != TWAI_ERROR_PASSIVE && canStatus.state != TWAI_ERROR_BUS_OFF)) {

            sendMessage(procOut, info);
        }


    } else if (procIn != nullptr) {

        removeOldMessages();
        readMessages();

        // search for matching message and copy them to info
        for (auto msg : receivedMessages) {
            if (msg.id == procIn->canId) {
                for (int i = 0; i + 2 < msg.len; i += 3) {
                    SignalType type = signalIdToType(msg.data[i]);
                    if (type != rcSignals::SignalType::ST_NONE) {
                        RcSignal value = (msg.data[i + 1] << 8) + msg.data[i + 2];
                        (*(info.signals))[type] = value;  // possibly overwrite old CAN value
                    }
                }
            }
        }
    }
}


OutputCan::OutputCan() :
  canId(0x35Du),
  types{SignalType::ST_WINCH, SignalType::ST_NONE } {
}

void OutputCan::start() {
    can.start(this);
}

void OutputCan::stop() {
    can.stop(this);
}

void OutputCan::step(const rcProc::StepInfo& info) {
    can.step(this, info);
}


} // namespace

