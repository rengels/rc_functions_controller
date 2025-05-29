/*
 * Mostly my own implementation of gatt services
 *
 */

/* Includes */
#include "bluetooth.h" // for the queue definitions

#include "gatt_svc.h"

#include <esp_log.h>

#include <assert.h>
#include <host/ble_hs.h>
#include <host/ble_uuid.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>
#include <services/ans/ble_svc_ans.h>

static const char* TAG = "BLE_GATT";

static const char *manuf_name = "OSS";
static const char *model_num = "0.1";

/** Signals (3.3.3.2.) Characteristic User Description */
static const char* const signals_user_descr = "An array of signed 16 bit values describing the internal controller signals.";

/** Signals (3.3.3.2.) Characteristic User Description */
static const char* const config_user_descr = "A binary encode stream containing the controller configuration.";

/** Signals (3.3.3.2.) Characteristic User Description */
static const char* const audio_user_descr = "A binary command to modify custom audio samples.";

/** Signals (3.3.3.2.) Characteristic User Description */
static const char* const audio_list_user_descr = "A binary stream containing a list of custom audio samples.";

#define GATT_DEVICE_INFO_UUID                 0x180A
#define GATT_MANUFACTURER_NAME_UUID           0x2A29
#define GATT_MODEL_NUMBER_UUID                0x2A24
#define GATT_CHAR_USER_DESCR_UUID             0x2901


/* Private function declarations */

uint16_t signals_chr_val_handle;
uint16_t config_chr_val_handle;
uint16_t audio_chr_val_handle;
uint16_t audio_list_chr_val_handle;

/** Arguments for the generic_chr_access function. */
struct GenericAccessArgs {
    char* characteristicsName;
    uint16_t* characteristicsValHandle;
    QueueHandle_t* inQueue;
    QueueHandle_t* outQueue;
};

/** Generic characteristic access function.
 *
 *  This function reads and writes to queues of QueueByteBuffers.
 *
 *  @param[in] arg  A struct of the type GenericAccessArgs
 */
static int generic_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg);

struct GenericAccessArgs signalsArgs = {"signals", &signals_chr_val_handle,
    &queueInSignals, &queueOutSignals};
struct GenericAccessArgs configArgs = {"config", &config_chr_val_handle,
    &queueInConfig, &queueOutConfig};
struct GenericAccessArgs audioArgs = {"audio", &audio_chr_val_handle,
    &queueInAudio, NULL};
struct GenericAccessArgs audioListArgs = {"audioList", &audio_list_chr_val_handle,
    NULL, &queueOutAudioList};

static int device_info_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg);

/** Generic characteristic access function.
 *
 *  This function reads and writes to queues of QueueByteBuffers.
 *
 *  @param[in] arg  A char* pointer to the description.
 */
static int generic_desc_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg);


static uint16_t signals_chr_conn_handle = 0;
static bool signals_chr_conn_handle_inited = false;
static bool signals_ind_status = false;


/// UUID for the rc signals service. Random
static const ble_uuid128_t signals_svc_uuid =
    BLE_UUID128_INIT(0x3f, 0x39, 0x2d, 0xb4, 0x40, 0x3a, 0x42, 0x20,
                     0xae, 0xdf, 0xd6, 0x04, 0x91, 0x2e, 0x52, 0x31);

/// UUID for the Signals characteristics. Random
static const ble_uuid128_t signals_chr_uuid =
    BLE_UUID128_INIT(0xa8, 0x56, 0xb5, 0xf9, 0xb3, 0x3f, 0x4f, 0x26,
                     0xb2, 0x50, 0x71, 0x2c, 0x81, 0x42, 0x7d, 0x17);

/// UUID for the rc config service. Random
static const ble_uuid128_t config_svc_uuid =
    BLE_UUID128_INIT(0x3f, 0x39, 0x2d, 0xb4, 0x40, 0x3a, 0x42, 0x20,
                     0xae, 0xdf, 0xd6, 0x04, 0x91, 0x2e, 0x52, 0x32);

/// UUID for the Config characteristics.
static const ble_uuid128_t config_chr_uuid =
    BLE_UUID128_INIT(0xa8, 0x56, 0xb5, 0xf9, 0xb3, 0x3f, 0x4f, 0x26,
                     0xb2, 0x50, 0x71, 0x2c, 0x81, 0x42, 0x7d, 0x18);

/// UUID for the audio characteristics.
static const ble_uuid128_t audio_chr_uuid =
    BLE_UUID128_INIT(0xa8, 0x56, 0xb5, 0xf9, 0xb3, 0x3f, 0x4f, 0x26,
                     0xb2, 0x50, 0x71, 0x2c, 0x81, 0x42, 0x7d, 0x19);

/// UUID for the audio characteristics.
static const ble_uuid128_t audio_list_chr_uuid =
    BLE_UUID128_INIT(0xa8, 0x56, 0xb5, 0xf9, 0xb3, 0x3f, 0x4f, 0x26,
                     0xb2, 0x50, 0x71, 0x2c, 0x81, 0x42, 0x7d, 0x1A);


/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {

    // Service: rc signals
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &signals_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]) { {
                // -- signals characteristics
                .uuid = &signals_chr_uuid.u,
                .access_cb = generic_chr_access,
                .arg = &signalsArgs,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_INDICATE,
                .descriptors =
                    (struct ble_gatt_dsc_def[]) { {
                        // characteristic user description (see: 3.3.3.2.)
                        .uuid = BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = generic_desc_chr_access,
                        .arg = (void*)signals_user_descr,
                    }, { 0, }  // No more descriptors in this characteristic
                    },
                .val_handle = &signals_chr_val_handle,
                .cpfd = NULL,  // client presentation format description
            }, { 0, }  // No more characteristics in this service.
            },
    },

    // Service: rc config
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &config_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]) { {
                // -- config (proc) characteristics
                .uuid = &config_chr_uuid.u,
                .access_cb = generic_chr_access,
                .arg = &configArgs,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                .descriptors =
                    (struct ble_gatt_dsc_def[]) { {
                        // characteristic user description (see: 3.3.3.2.)
                        .uuid = BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = generic_desc_chr_access,
                        .arg = (void*)config_user_descr,
                    }, { 0, } }, // No more descriptors in this characteristic
                .val_handle = &config_chr_val_handle,
                .cpfd = NULL,  // client presentation format description
            },

            // -- audio file characteristics
            {
                .uuid = &audio_chr_uuid.u,
                .access_cb = generic_chr_access,
                .arg = &audioArgs,
                .flags =  BLE_GATT_CHR_F_WRITE,
                .descriptors =
                    (struct ble_gatt_dsc_def[]) { {
                        // characteristic user description (see: 3.3.3.2.)
                        .uuid = BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = generic_desc_chr_access,
                        .arg = (void*)audio_user_descr,
                    }, { 0, } }, // No more descriptors in this characteristic
                .val_handle = &audio_chr_val_handle,
                .cpfd = NULL,  // client presentation format description
            },

            // -- audio list characteristics
            {
                .uuid = &audio_list_chr_uuid.u,
                .access_cb = generic_chr_access,
                .arg = &audioListArgs,
                .flags = BLE_GATT_CHR_F_READ,
                .descriptors =
                    (struct ble_gatt_dsc_def[]) { {
                        // characteristic user description (see: 3.3.3.2.)
                        .uuid = BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = generic_desc_chr_access,
                        .arg = (void*)audio_list_user_descr,
                    }, { 0, } },  // No more descriptors in this characteristic
                .val_handle = &audio_list_chr_val_handle,
                .cpfd = NULL,  // client presentation format description
            },
            { 0, } },  // No more characteristics in this service.
    },

    // Service: Device Information
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
              // Characteristic: * Manufacturer name
              .uuid = BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID),
              .access_cb = device_info_chr_access,
              .flags = BLE_GATT_CHR_F_READ,
          }, {
              // Characteristic: Model number string
              .uuid = BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID),
              .access_cb = device_info_chr_access,
              .flags = BLE_GATT_CHR_F_READ,
          }, { 0, }, // No more characteristics in this service
        }
    },

    { 0, },  // No more services.
};


/** Peeks on the given queue and appends it to the ctxt for sending. */
static int sendTopQueue(QueueHandle_t queue, struct ble_gatt_access_ctxt *ctxt) {

    QueueByteBuffer buffer;
    if (xQueuePeek(queue, (void*)&buffer, (TickType_t)0)) {
        ESP_LOGI(TAG, "sendTopQueue; 1=%c 2=%c len=%d", buffer.data[0], buffer.data[1], buffer.len);
        int rc = os_mbuf_append(ctxt->om,
                            buffer.data,
                            buffer.len);

        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_READ_NOT_PERMITTED;
}


/** Copies the mbuf to the queue and send's it.
 */
static int receiveToQueue(QueueHandle_t queue, struct ble_gatt_access_ctxt *ctxt) {

    uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);

    QueueByteBuffer buffer = {
        .data = malloc(om_len),
        .len = om_len
    };
    if (buffer.data == NULL) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    uint16_t actualLen = 0;
    int rc = ble_hs_mbuf_to_flat(ctxt->om, buffer.data, buffer.len, &actualLen);
    ESP_LOGI(TAG, "receiveToQueue; omLen=%u len=%u rc=%d", om_len, actualLen, rc);

    if (rc != 0) {
        free(buffer.data);
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (xQueueSendToBack(queue, (void*)&buffer, (TickType_t)0) != pdPASS) {
        free(buffer.data);
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}



/* Private functions */

/* Public functions */
void send_signals_indication(void) {
    if (signals_ind_status && signals_chr_conn_handle_inited) {
        ble_gatts_indicate(signals_chr_conn_handle, signals_chr_val_handle);
        // ESP_LOGI(TAG, "signals indication sent!");
    }
}

/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 *
 *  Note: this will print out status information for the characteristics
 *    we register with Nimble stack.
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

/*
 *  GATT server subscribe event callback
 *      1. Update heart rate subscription status
 */
void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    if (event->subscribe.attr_handle == signals_chr_val_handle) {
        // update subscription status (TODO: handle several subscriptions
        signals_chr_conn_handle = event->subscribe.conn_handle;
        signals_chr_conn_handle_inited = true;
        signals_ind_status = event->subscribe.cur_indicate;
    }
}


static int generic_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {

    struct GenericAccessArgs* aArgs = (struct GenericAccessArgs*)arg;

    if (attr_handle != *(aArgs->characteristicsValHandle)) {
        ESP_LOGE(TAG, "unexcpected handle: %d", attr_handle);
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "Descriptor read; %s conn_handle=%d attr_handle=%d",
                        aArgs->characteristicsName, conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "Descriptor read by NimBLE stack; %s attr_handle=%d",
                        aArgs->characteristicsName, attr_handle);
        }

        if (aArgs->outQueue != NULL) {
            return sendTopQueue(*(aArgs->outQueue), ctxt);
        } else {
            return BLE_ATT_ERR_READ_NOT_PERMITTED;
        }

    } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "Descriptor write; %s conn_handle=%d attr_handle=%d",
                        aArgs->characteristicsName, conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "Descriptor write by NimBLE stack; %s attr_handle=%d",
                        aArgs->characteristicsName, attr_handle);
        }

        if (aArgs->inQueue != NULL) {
            return receiveToQueue(*(aArgs->inQueue), ctxt);
        } else {
            return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
        }

    } else {
        ESP_LOGE(TAG,
            "unexpected access operation to audio_list characteristic, opcode: %d",
            ctxt->op);
        return BLE_ATT_ERR_UNLIKELY;
    }
}


static int generic_desc_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC) {
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "Descriptor read; conn_handle=%d attr_handle=%d",
                        conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "Descriptor read by NimBLE stack; attr_handle=%d",
                        attr_handle);
        }

        if (ble_uuid_cmp(ctxt->dsc->uuid,
            BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID)) == 0) {

            ESP_LOGI(TAG, "Descriptor read User descriptor\n");
            int rc = os_mbuf_append(ctxt->om,
                                arg,
                                strlen(arg));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return BLE_ATT_ERR_UNLIKELY;

    } else {
        ESP_LOGW(TAG, "Unsupported operation. %d", ctxt->op);
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
}


static int device_info_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
    uint16_t uuid = ble_uuid_u16(ctxt->chr->uuid);

    ESP_LOGD(TAG, "access device info with handle=%d", attr_handle);

    if (uuid == GATT_MODEL_NUMBER_UUID) {
        int rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_MANUFACTURER_NAME_UUID) {
        int rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return BLE_ATT_ERR_UNLIKELY;
}



/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
