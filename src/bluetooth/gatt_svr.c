/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "bleprph.h"
#include "bluetooth.h" // for the queue definitions

#include <esp_log.h>

#include <assert.h>
#include <host/ble_hs.h>
#include <host/ble_uuid.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>
#include <services/ans/ble_svc_ans.h>

QueueHandle_t queueOutSignals = NULL;
QueueHandle_t queueInSignals = NULL;
QueueHandle_t queueOutConfig = NULL;
QueueHandle_t queueInConfig = NULL;
QueueHandle_t queueInAudio = NULL;
QueueHandle_t queueOutAudioList = NULL;

static const char *tag = "BLE_GATT";
static const char *manuf_name = "OSS";
static const char *model_num = "0.1";

/* Levels:
 *
 * - server
 *   - service
 *     - characteristics
 *       - descriptor
 */

/// UUID for the rc signals service. Random
static const ble_uuid128_t gatt_svr_svc_signals_uuid =
    BLE_UUID128_INIT(0x3f, 0x39, 0x2d, 0xb4, 0x40, 0x3a, 0x42, 0x20,
                     0xae, 0xdf, 0xd6, 0x04, 0x91, 0x2e, 0x52, 0x31);

/// UUID for the Signals characteristics. Random
static const ble_uuid128_t gatt_chr_signals_uuid =
    BLE_UUID128_INIT(0xa8, 0x56, 0xb5, 0xf9, 0xb3, 0x3f, 0x4f, 0x26,
                     0xb2, 0x50, 0x71, 0x2c, 0x81, 0x42, 0x7d, 0x17);

/// UUID for the rc config service. Random
static const ble_uuid128_t gatt_svr_svc_config_uuid =
    BLE_UUID128_INIT(0x3f, 0x39, 0x2d, 0xb4, 0x40, 0x3a, 0x42, 0x20,
                     0xae, 0xdf, 0xd6, 0x04, 0x91, 0x2e, 0x52, 0x32);

/// UUID for the Config characteristics.
static const ble_uuid128_t gatt_chr_config_uuid =
    BLE_UUID128_INIT(0xa8, 0x56, 0xb5, 0xf9, 0xb3, 0x3f, 0x4f, 0x26,
                     0xb2, 0x50, 0x71, 0x2c, 0x81, 0x42, 0x7d, 0x18);

/// UUID for the audio characteristics.
static const ble_uuid128_t gatt_chr_audio_uuid =
    BLE_UUID128_INIT(0xa8, 0x56, 0xb5, 0xf9, 0xb3, 0x3f, 0x4f, 0x26,
                     0xb2, 0x50, 0x71, 0x2c, 0x81, 0x42, 0x7d, 0x19);

/// UUID for the audio characteristics.
static const ble_uuid128_t gatt_chr_audio_list_uuid =
    BLE_UUID128_INIT(0xa8, 0x56, 0xb5, 0xf9, 0xb3, 0x3f, 0x4f, 0x26,
                     0xb2, 0x50, 0x71, 0x2c, 0x81, 0x42, 0x7d, 0x1A);

uint16_t gatt_svr_chr1_val_handle;
uint16_t gatt_svr_chr2_val_handle;
uint16_t gatt_svr_chr3_val_handle;
uint16_t gatt_svr_chr4_val_handle;

/** Signals (3.3.3.2.) Characteristic User Description */
static const char* signals_user_descr1 = "An array of signed 16 bit values describing the internal controller signals.";

/** Signals (3.3.3.2.) Characteristic User Description */
static const char* signals_user_descr2 = "A binary encode stream containing the controller configuration.";

/** Signals (3.3.3.2.) Characteristic User Description */
static const char* signals_user_descr3 = "A binary command to modify custom audio samples.";

/** Signals (3.3.3.2.) Characteristic User Description */
static const char* signals_user_descr4 = "A binary stream containing a list of custom audio samples.";


#define GATT_DEVICE_INFO_UUID                 0x180A
#define GATT_MANUFACTURER_NAME_UUID           0x2A29
#define GATT_MODEL_NUMBER_UUID                0x2A24
#define GATT_CHAR_USER_DESCR_UUID             0x2901

/** handle for manufacturer name characteristics */
uint16_t gatt_svr_chr4_val_handle;

/** handle for model name characteristics */
uint16_t gatt_svr_chr5_val_handle;

static int
gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle,
                struct ble_gatt_access_ctxt *ctxt,
                void *arg);

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);


/** _Rc functions controller_ Service
 *
 *  This service has the _signals and the _config_ characteristic.
 */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {

    // Service: rc signals
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_signals_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[])
        {
        // -- signals characteristics
        {
            // This characteristic can be subscribed to by writing 0x00 and 0x01 to the CCCD
            .uuid = &gatt_chr_signals_uuid.u,
            .access_cb = gatt_svc_access,
#if CONFIG_EXAMPLE_ENCRYPTION
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_WRITE_ENC |
                BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_INDICATE,
#else
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_INDICATE,
#endif
            .val_handle = &gatt_svr_chr1_val_handle,
            .descriptors = (struct ble_gatt_dsc_def[])
            { {
                  // characteristic user description (see: 3.3.3.2.)
                  .uuid = BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID),
#if CONFIG_EXAMPLE_ENCRYPTION
                  .att_flags = BLE_ATT_F_READ | BLE_ATT_F_READ_ENC,
#else
                  .att_flags = BLE_ATT_F_READ,
#endif
                  .access_cb = gatt_svc_access,
              }, {
                  0, // No more descriptors in this characteristic
              }
            },
        },
        {
            0, // No more characteristics in this service.
        }
        },
    },

    // Service: rc config
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_config_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[])
        {
        // -- config (proc) characteristics
        {
            .uuid = &gatt_chr_config_uuid.u,
            .access_cb = gatt_svc_access,
#if CONFIG_EXAMPLE_ENCRYPTION
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_WRITE_ENC,
#else
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
#endif
            .val_handle = &gatt_svr_chr2_val_handle,
            .descriptors = (struct ble_gatt_dsc_def[])
            { {
                  // characteristic user description (see: 3.3.3.2.)
                  .uuid = BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID),
#if CONFIG_EXAMPLE_ENCRYPTION
                  .att_flags = BLE_ATT_F_READ | BLE_ATT_F_READ_ENC,
#else
                  .att_flags = BLE_ATT_F_READ,
#endif
                  .access_cb = gatt_svc_access,
              }, {
                  0, // No more descriptors in this characteristic
              }
            },
        },

        // -- audio file characteristics
        {
            .uuid = &gatt_chr_audio_uuid.u,
            .access_cb = gatt_svc_access,
#if CONFIG_EXAMPLE_ENCRYPTION
            .flags =  BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
#else
            .flags =  BLE_GATT_CHR_F_WRITE,
#endif
            .val_handle = &gatt_svr_chr3_val_handle,
            .descriptors = (struct ble_gatt_dsc_def[])
            { {
                  // characteristic user description (see: 3.3.3.2.)
                  .uuid = BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID),
#if CONFIG_EXAMPLE_ENCRYPTION
                  .att_flags = BLE_ATT_F_READ | BLE_ATT_F_READ_ENC,
#else
                  .att_flags = BLE_ATT_F_READ,
#endif
                  .access_cb = gatt_svc_access,
              }, {
                  0, // No more descriptors in this characteristic
              }
            },
        },

        // -- audio list characteristics
        {
            .uuid = &gatt_chr_audio_list_uuid.u,
            .access_cb = gatt_svc_access,
#if CONFIG_EXAMPLE_ENCRYPTION
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
#else
            .flags = BLE_GATT_CHR_F_READ,
#endif
            .val_handle = &gatt_svr_chr4_val_handle,
            .descriptors = (struct ble_gatt_dsc_def[])
            { {
                  // characteristic user description (see: 3.3.3.2.)
                  .uuid = BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID),
#if CONFIG_EXAMPLE_ENCRYPTION
                  .att_flags = BLE_ATT_F_READ | BLE_ATT_F_READ_ENC,
#else
                  .att_flags = BLE_ATT_F_READ,
#endif
                  .access_cb = gatt_svc_access,
              }, {
                  0, // No more descriptors in this characteristic
              }
            },
        },
        {
            0, // No more characteristics in this service.
        }
        },
    },

    // Service: Device Information
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
              // Characteristic: * Manufacturer name
              .uuid = BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID),
              .access_cb = gatt_svr_chr_access_device_info,
              .flags = BLE_GATT_CHR_F_READ,
          }, {
              // Characteristic: Model number string
              .uuid = BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID),
              .access_cb = gatt_svr_chr_access_device_info,
              .flags = BLE_GATT_CHR_F_READ,
          }, {
              0, // No more characteristics in this service
          },
        }
    },

    {
        0, // No more services.
    },
};


/** Peeks on the given queue and appends it to the ctxt for sending. */
static int sendTopQueue(QueueHandle_t queue, struct ble_gatt_access_ctxt *ctxt) {

    QueueByteBuffer buffer;
    if (xQueuePeek(queue, (void*)&buffer, (TickType_t)0)) {
        ESP_LOGI(tag, "sendTopQueue; 1=%c 2=%c len=%d\n", buffer.data[0], buffer.data[1], buffer.len);
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
    ESP_LOGI(tag, "receiveToQueue; omLen=%u len=%u rc=%d\n", om_len, actualLen, rc);

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

/**
 * Access callback whenever a characteristic/descriptor is read or written to.
 * Here reads and writes need to be handled.
 * ctxt->op tells weather the operation is read or write and
 * weather it is on a characteristic or descriptor,
 * ctxt->dsc->uuid tells which characteristic/descriptor is accessed.
 * attr_handle give the value handle of the attribute being accessed.
 * Accordingly do:
 *     Append the value to ctxt->om if the operation is READ
 *     Write ctxt->om to the value if the operation is WRITE
 **/
static int
gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle,
                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const ble_uuid_t *uuid;
    int rc;

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(tag, "Characteristic read; conn_handle=%d attr_handle=%d\n",
                        conn_handle, attr_handle);
        } else {
            ESP_LOGI(tag, "Characteristic read by NimBLE stack; attr_handle=%d\n",
                        attr_handle);
        }
        uuid = ctxt->chr->uuid;

        // -- signals
        if (attr_handle == gatt_svr_chr1_val_handle) {
            return sendTopQueue(queueOutSignals, ctxt);

        // -- config
        } else if (attr_handle == gatt_svr_chr2_val_handle) {
            return sendTopQueue(queueOutConfig, ctxt);

        // -- audio file
        } else if (attr_handle == gatt_svr_chr3_val_handle) {
            return BLE_ATT_ERR_READ_NOT_PERMITTED;

        // -- audio list
        } else if (attr_handle == gatt_svr_chr4_val_handle) {
            return sendTopQueue(queueOutAudioList, ctxt);

        } else {
            ESP_LOGW(tag, "Trying to read unknown handle; conn_handle=%d attr_handle=%d",
                        conn_handle, attr_handle);
            return BLE_ATT_ERR_READ_NOT_PERMITTED;
        }

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(tag, "Characteristic write; conn_handle=%d attr_handle=%d",
                        conn_handle, attr_handle);
        } else {
            ESP_LOGI(tag, "Characteristic write by NimBLE stack; attr_handle=%d",
                        attr_handle);
        }
        uuid = ctxt->chr->uuid;

        // -- signals
        if (attr_handle == gatt_svr_chr1_val_handle) {
            ESP_LOGI(tag, "Received new signals");
            rc = receiveToQueue(queueInSignals, ctxt);

            ble_gatts_chr_updated(attr_handle);
            return rc;

        // -- config (proc)
        } else if (attr_handle == gatt_svr_chr2_val_handle) {
            ESP_LOGI(tag, "Received new config");
            rc = receiveToQueue(queueInConfig, ctxt);
            // ble_gatts_chr_updated(attr_handle);
            return rc;

        // -- audio
        } else if (attr_handle == gatt_svr_chr3_val_handle) {
            ESP_LOGI(tag, "Received new audio");
            rc = receiveToQueue(queueInAudio, ctxt);
            // ble_gatts_chr_updated(attr_handle);
            return rc;

        } else {
            ESP_LOGW(tag, "Trying to write unknown handle; conn_handle=%d attr_handle=%d",
                        conn_handle, attr_handle);
            return BLE_ATT_ERR_READ_NOT_PERMITTED;
        }
        ESP_LOGI(tag, "Write done");

    case BLE_GATT_ACCESS_OP_READ_DSC:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(tag, "Descriptor read; conn_handle=%d attr_handle=%d\n",
                        conn_handle, attr_handle);
        } else {
            ESP_LOGI(tag, "Descriptor read by NimBLE stack; attr_handle=%d\n",
                        attr_handle);
        }
        uuid = ctxt->dsc->uuid;
        // read user descriptor
        if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(GATT_CHAR_USER_DESCR_UUID)) == 0) {
            ESP_LOGI(tag, "Descriptor read User descriptor\n");
            if (attr_handle == 29) {
                rc = os_mbuf_append(ctxt->om,
                                    signals_user_descr1,
                                    strlen(signals_user_descr1));
              ESP_LOGI(tag, "Descriptor read for signals\n");
            } else if (attr_handle == 30) {
                rc = os_mbuf_append(ctxt->om,
                                    signals_user_descr2,
                                    strlen(signals_user_descr2));
            } else if (attr_handle == 31) {
                rc = os_mbuf_append(ctxt->om,
                                    signals_user_descr3,
                                    strlen(signals_user_descr3));
            } else if (attr_handle == 32) {
                rc = os_mbuf_append(ctxt->om,
                                    signals_user_descr4,
                                    strlen(signals_user_descr4));
            } else {
                rc = 0;
            }
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        } else {
            ESP_LOGW(tag, "Trying to read unknown descriptor; conn_handle=%d attr_handle=%d",
                        conn_handle, attr_handle);
            return BLE_ATT_ERR_READ_NOT_PERMITTED;
        }

    case BLE_GATT_ACCESS_OP_WRITE_DSC:
        // we don't support writing descriptors
        ESP_LOGW(tag, "We don't support writing descriptors.");
        return BLE_ATT_ERR_READ_NOT_PERMITTED;

    default:
        ESP_LOGW(tag, "Unsupported operation.");
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
}

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {

    char buffer[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(tag, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buffer),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(tag, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buffer),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(tag, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buffer),
                    ctxt->dsc.handle);
        break;

    default:
        ESP_LOGW(tag, "Unsupported operation %d in gatt_svr_register_cb.",
             ctxt->op);
        break;
    }
}

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    ESP_LOGD(tag, "access device info with handle=%d\n", attr_handle);

    if (uuid == GATT_MODEL_NUMBER_UUID) {
        rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_MANUFACTURER_NAME_UUID) {
        rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return BLE_ATT_ERR_UNLIKELY;
}


int gatt_svr_init(void) {
    int rc;

    // setup my queues
    queueOutSignals   = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueInSignals    = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueOutConfig    = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueInConfig     = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueInAudio      = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueOutAudioList = xQueueCreate(1, sizeof(QueueByteBuffer));

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
