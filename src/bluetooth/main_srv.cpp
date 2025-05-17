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

#include <esp_log.h>

/* BLE */
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <host/ble_hs.h>
#include <host/util/util.h>
#include <console/console.h>
#include <services/gap/ble_svc_gap.h>

#include <esp_mac.h>
#include <esp_crc.h>

#include "bluetooth.h"
#include "bleprph.h"

#define GATT_SVR_SVC_ALERT_UUID               0x1811

#if CONFIG_EXAMPLE_EXTENDED_ADV
static uint8_t ext_adv_pattern_1[] = {
    0x02, 0x01, 0x06,
    0x03, 0x03, 0xab, 0xcd,
    0x03, 0x03, 0x18, 0x11,
    0x11, 0X09, 'n', 'i', 'm', 'b', 'l', 'e', '-', 'b', 'l', 'e', 'p', 'r', 'p', 'h', '-', 'e',
};
#endif

/** Timer handle for the signals notification timer. */
static TimerHandle_t signals_notification_timer;

/** True if some-one registered for notifications. */
static bool signals_notification_subscribed;

/** Connection handle for the connection */
static uint16_t conn_handle;

static const char *tag = "NimBLE";
static int bleprph_gap_event(struct ble_gap_event *event, void *arg);
#if CONFIG_EXAMPLE_RANDOM_ADDR
static uint8_t own_addr_type = BLE_OWN_ADDR_RANDOM;
#else
static uint8_t own_addr_type;
#endif

extern "C" {
    void ble_store_config_init(void);
}

// forward declarations
static void signals_notify_stop();
static void signals_notify_reset();

/**
 * Logs information about a connection to the console.
 */
static void bleprph_print_conn_desc(struct ble_gap_conn_desc *desc) {

    ESP_LOGI(tag, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    print_addr(desc->our_ota_addr.val);
    ESP_LOGI(tag, " our_id_addr_type=%d our_id_addr=",
                desc->our_id_addr.type);
    print_addr(desc->our_id_addr.val);
    ESP_LOGI(tag, " peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    print_addr(desc->peer_ota_addr.val);
    ESP_LOGI(tag, " peer_id_addr_type=%d peer_id_addr=",
                desc->peer_id_addr.type);
    print_addr(desc->peer_id_addr.val);
    ESP_LOGI(tag, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d\n",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);

}

#if CONFIG_EXAMPLE_EXTENDED_ADV
/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void
ext_bleprph_advertise(void)
{
    struct ble_gap_ext_adv_params params;
    struct os_mbuf *data;
    uint8_t instance = 0;
    int rc;

    /* First check if any instance is already active */
    if(ble_gap_ext_adv_active(instance)) {
        return;
    }

    /* use defaults for non-set params */
    memset (&params, 0, sizeof(params));

    /* enable connectable advertising */
    params.connectable = 1;

    /* advertise using random addr */
    params.own_addr_type = BLE_OWN_ADDR_PUBLIC;

    params.primary_phy = BLE_HCI_LE_PHY_1M;
    params.secondary_phy = BLE_HCI_LE_PHY_2M;
    //params.tx_power = 127;
    params.sid = 1;

    params.itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN;
    params.itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MIN;

    /* configure instance 0 */
    rc = ble_gap_ext_adv_configure(instance, &params, NULL,
                                   bleprph_gap_event, NULL);
    assert (rc == 0);

    /* in this case only scan response is allowed */

    /* get mbuf for scan rsp data */
    data = os_msys_get_pkthdr(sizeof(ext_adv_pattern_1), 0);
    assert(data);

    /* fill mbuf with scan rsp data */
    rc = os_mbuf_append(data, ext_adv_pattern_1, sizeof(ext_adv_pattern_1));
    assert(rc == 0);

    rc = ble_gap_ext_adv_set_data(instance, data);
    assert (rc == 0);

    /* start advertising */
    rc = ble_gap_ext_adv_start(instance, 0, 0);
    assert (rc == 0);
}
#else
/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void
bleprph_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name = ble_svc_gap_device_name();
    int rc;

    ble_uuid16_t uuids16[] = {
        {.u = {.type = BLE_UUID_TYPE_16},
         .value = GATT_SVR_SVC_ALERT_UUID,}
    };

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    memset(&fields, 0, sizeof fields);

    /* Advertise two flags:
     *     o Discoverability in forthcoming advertisement (general)
     *     o BLE-only (BR/EDR unsupported).
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.uuids16 = uuids16,
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(tag, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, bleprph_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(tag, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}
#endif

#if MYNEWT_VAL(BLE_POWER_CONTROL)
static void bleprph_power_control(uint16_t conn_handle)
{
    int rc;

    rc = ble_gap_read_remote_transmit_power_level(conn_handle, 0x01 );  // Attempting on LE 1M phy
    assert (rc == 0);

    rc = ble_gap_set_transmit_power_reporting_enable(conn_handle, 0x1, 0x1);
    assert (rc == 0);
}
#endif

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event       The type of event being signalled.
 * @param ctxt        Various information pertaining to the event.
 * @param arg         Application-specified argument; unused by
 *                        bleprph.
 *
 * @return            0 if the application successfully handled the
 *                        event; nonzero on failure.  The semantics
 *                        of the return code is specific to the
 *                        particular GAP event being signalled.
 */
static int
bleprph_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    ESP_LOGI(tag, "gap_event: %d", event->type);

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        // A new connection was established or a connection attempt failed.
        ESP_LOGI(tag, "connection %s; status=%d ",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            bleprph_print_conn_desc(&desc);
        }

        if (event->connect.status != 0) {
            // Connection failed; resume advertising.
#if CONFIG_EXAMPLE_EXTENDED_ADV
            ext_bleprph_advertise();
#else
            bleprph_advertise();
#endif
        }
        conn_handle = event->connect.conn_handle;


#if MYNEWT_VAL(BLE_POWER_CONTROL)
	bleprph_power_control(event->connect.conn_handle);
#endif
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(tag, "disconnect; reason=%d ", event->disconnect.reason);
        bleprph_print_conn_desc(&event->disconnect.conn);

        // invalidate all signals
        // TODO

        // Connection terminated; resume advertising.
#if CONFIG_EXAMPLE_EXTENDED_ADV
        ext_bleprph_advertise();
#else
        bleprph_advertise();
#endif
        signals_notification_subscribed = false;
        conn_handle = 0;
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        ESP_LOGI(tag, "connection updated; status=%d ",
                    event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        bleprph_print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(tag, "advertise complete; reason=%d",
                    event->adv_complete.reason);
#if CONFIG_EXAMPLE_EXTENDED_ADV
        ext_bleprph_advertise();
#else
        bleprph_advertise();
#endif
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        ESP_LOGI(tag, "encryption change event; status=%d ",
                    event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        bleprph_print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_NOTIFY_TX:
        ESP_LOGD(tag, "notify_tx event; conn_handle=%d attr_handle=%d "
                    "status=%d is_indication=%d",
                    event->notify_tx.conn_handle,
                    event->notify_tx.attr_handle,
                    event->notify_tx.status,
                    event->notify_tx.indication);
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(tag, "subscribe event; conn_handle=%d attr_handle=%d "
                    "reason=%d prevn=%d curn=%d previ=%d curi=%d",
                    event->subscribe.conn_handle,
                    event->subscribe.attr_handle,
                    event->subscribe.reason,
                    event->subscribe.prev_notify,
                    event->subscribe.cur_notify,
                    event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);

        if (event->subscribe.attr_handle == gatt_svr_chr1_val_handle) {
            ESP_LOGI(tag, "Subscribed to signals");
            signals_notification_subscribed = event->subscribe.cur_notify;
            signals_notify_reset();
        } else {
            ESP_LOGI(tag, "Subscription to something else");
            signals_notification_subscribed = event->subscribe.cur_notify;
            signals_notify_reset();
        }
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(tag, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    case BLE_GAP_EVENT_PASSKEY_ACTION:
        {
            ESP_LOGI(tag, "PASSKEY_ACTION_EVENT started");
            struct ble_sm_io pkey = {.action = 0, .passkey = 0};
            int key = 0;

            if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
                pkey.action = event->passkey.params.action;
                pkey.passkey = 123456; // This is the passkey to be entered on peer
                ESP_LOGI(tag, "Enter passkey %" PRIu32 "on the peer side", pkey.passkey);
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                ESP_LOGI(tag, "ble_sm_inject_io result: %d", rc);
            } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
                ESP_LOGI(tag, "Passkey on device's display: %" PRIu32 , event->passkey.params.numcmp);
                ESP_LOGI(tag, "Accept or reject the passkey through console in this format -> key Y or key N");
                pkey.action = event->passkey.params.action;
                if (/* TODO scli_receive_key(&key)*/ false) {
                    pkey.numcmp_accept = key;
                } else {
                    pkey.numcmp_accept = 0;
                    ESP_LOGE(tag, "Timeout! Rejecting the key");
                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                ESP_LOGI(tag, "ble_sm_inject_io result: %d", rc);
            } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
                static uint8_t tem_oob[16] = {0};
                pkey.action = event->passkey.params.action;
                for (int i = 0; i < 16; i++) {
                    pkey.oob[i] = tem_oob[i];
                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                ESP_LOGI(tag, "ble_sm_inject_io result: %d", rc);
            } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
                ESP_LOGI(tag, "Enter the passkey through console in this format-> key 123456");
                pkey.action = event->passkey.params.action;
                if (/* TODO scli_receive_key(&key) */ false) {
                    pkey.passkey = key;
                } else {
                    pkey.passkey = 0;
                    ESP_LOGE(tag, "Timeout! Passing 0 as the key");
                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                ESP_LOGI(tag, "ble_sm_inject_io result: %d", rc);
            }
        }
        return 0;


    case BLE_GAP_EVENT_AUTHORIZE:
        ESP_LOGI(tag, "authorize event: conn_handle=%d attr_handle=%d is_read=%d",
                    event->authorize.conn_handle,
                    event->authorize.attr_handle,
                    event->authorize.is_read);

        /* The default behaviour for the event is to reject authorize request */
        event->authorize.out_response = BLE_GAP_AUTHORIZE_REJECT;
        return 0;

#if MYNEWT_VAL(BLE_POWER_CONTROL)
    case BLE_GAP_EVENT_TRANSMIT_POWER:
        ESP_LOGI(tag, "Transmit power event : status=%d conn_handle=%d reason=%d "
                           "phy=%d power_level=%x power_level_flag=%d delta=%d",
                     event->transmit_power.status,
                     event->transmit_power.conn_handle,
                     event->transmit_power.reason,
                     event->transmit_power.phy,
                     event->transmit_power.transmit_power_level,
                     event->transmit_power.transmit_power_level_flag,
                     event->transmit_power.delta);
        return 0;

     case BLE_GAP_EVENT_PATHLOSS_THRESHOLD:
        ESP_LOGI(tag, "Pathloss threshold event : conn_handle=%d current path loss=%d "
                           "zone_entered =%d",
                     event->pathloss_threshold.conn_handle,
                     event->pathloss_threshold.current_path_loss,
                     event->pathloss_threshold.zone_entered);
        return 0;
#endif
     case BLE_GAP_EVENT_DATA_LEN_CHG:
        ESP_LOGI(tag, "data len event : conn_handle=%d tx: max octets=%d max time=%d"
                      "rx: max octets=%d max time=%d",
                     event->data_len_chg.conn_handle,
                     event->data_len_chg.max_tx_octets,
                     event->data_len_chg.max_tx_time,
                     event->data_len_chg.max_rx_octets,
                     event->data_len_chg.max_rx_time);
        rc = ble_gap_set_data_len(
            event->data_len_chg.conn_handle,
            event->data_len_chg.max_tx_octets,
            event->data_len_chg.max_tx_time);

        return 0;
    }

    return 0;
}

static void bleprph_on_reset(int reason) {

    ESP_LOGI(tag, "Resetting state; reason=%d\n", reason);
}

#if CONFIG_EXAMPLE_RANDOM_ADDR
static void ble_app_set_addr(void) {

    ble_addr_t addr;
    int rc;

    /* generate new non-resolvable private address */
    rc = ble_hs_id_gen_rnd(0, &addr);
    assert(rc == 0);

    /* set generated address */
    rc = ble_hs_id_set_rnd(addr.val);

    assert(rc == 0);
}
#endif

static void bleprph_on_sync(void) {

    int rc;

#if CONFIG_EXAMPLE_RANDOM_ADDR
    /* Generate a non-resolvable private address. */
    ble_app_set_addr();
#endif

    /* Make sure we have proper identity address set (public preferred) */
#if CONFIG_EXAMPLE_RANDOM_ADDR
    rc = ble_hs_util_ensure_addr(1);
#else
    rc = ble_hs_util_ensure_addr(0);
#endif
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(tag, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    ESP_LOGI(tag, "Device Address: ");
    print_addr(addr_val);
    ESP_LOGI(tag, "\n");
    /* Begin advertising. */
#if CONFIG_EXAMPLE_EXTENDED_ADV
    ext_bleprph_advertise();
#else
    bleprph_advertise();
#endif
}


static void signals_notify_stop()
{
    ESP_LOGI(tag, "signals_notify_stop");
    xTimerStop(signals_notification_timer, 1000 / portTICK_PERIOD_MS);
}

/* Reset signals update timer */
static void signals_notify_reset()
{
    if (xTimerReset(signals_notification_timer, 1000 / portTICK_PERIOD_MS) == pdPASS) {
    } else {
        ESP_LOGW(tag, "Unable to reset notification timer.");
    }
}

/** Sends out the current signals as a notification */
static void signals_send_update(TimerHandle_t ev)
{
    if (!signals_notification_subscribed) {
        signals_notify_stop();
        return;
    }

    QueueByteBuffer buffer;
    if (xQueuePeek(queueOutSignals, &buffer, (TickType_t)0)) {
        struct os_mbuf *om;
        om = ble_hs_mbuf_from_flat(buffer.data, buffer.len);

        ESP_ERROR_CHECK(
            ble_gatts_notify_custom(conn_handle, gatt_svr_chr1_val_handle, om));
    }

    signals_notify_reset();
}

/** Bluetooth nimble task.
 *
 * This function will return only when nimble_port_stop() is executed
 */
void bleHostTask(void *param) {
    ESP_LOGI(tag, "BLE Host Task Started");
    nimble_port_run();

    nimble_port_freertos_deinit();
}

/** Returns a static pointer to the device name.
 *
 *  In order to use multiple function controllers, we would lise
 *  to have a unique device name for each of them.
 *
 *  We use the MAC address to compute one.
 */
char* deviceName() {
    static char name[] = {'R', 'c', 'F', 'u', 'n', 'c', 'C', 't', 'r', 'l', '-', 0, 0, 0};

    // get a unique but stable number via a CRC of the MAC
    uint8_t mac[10] = {0};  // either 6 or 8 bytes (10 just to be on the safe side)
    ESP_ERROR_CHECK(
        esp_efuse_mac_get_default(mac));
    uint16_t crc = esp_crc16_le(UINT16_MAX, mac, 8);

    // now get some characters from the CRC
    const char* chars = "0123456789abcdefghiojklmnopqrstuvwxyz";
    name[11] = chars[crc % 32];
    name[12] = chars[(crc / 32) % 32];

    return name;
}

void btStart(void) {
    ESP_ERROR_CHECK(
        nimble_port_init());

    /* Initialize the NimBLE host configuration. */
    ble_hs_cfg.reset_cb = bleprph_on_reset;
    ble_hs_cfg.sync_cb = bleprph_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = CONFIG_EXAMPLE_IO_TYPE;
#ifdef CONFIG_EXAMPLE_BONDING
    ble_hs_cfg.sm_bonding = 1;
    /* Enable the appropriate bit masks to make sure the keys
     * that are needed are exchanged
     */
    ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
    ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
#endif
#ifdef CONFIG_EXAMPLE_MITM
    ble_hs_cfg.sm_mitm = 1;
#endif
#ifdef CONFIG_EXAMPLE_USE_SC
    ble_hs_cfg.sm_sc = 1;
#else
    ble_hs_cfg.sm_sc = 0;
#endif
#ifdef CONFIG_EXAMPLE_RESOLVE_PEER_ADDR
    /* Stores the IRK */
    ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
#endif

    ESP_ERROR_CHECK(
        gatt_svr_init());

    ESP_ERROR_CHECK(
        ble_svc_gap_device_name_set(deviceName()));

    ble_store_config_init();

    nimble_port_freertos_init(bleHostTask);

    signals_notification_timer = xTimerCreate(
        "signals_notification_timer",
        pdMS_TO_TICKS(500),
        pdTRUE,
        (void *)0,
        signals_send_update);
}

void btStop(void) {
    nimble_port_stop();
}

