/*
 * The original file is:
 *   examples/bluetooth/ble_get_started/nimble/NimBLE_GATT_Server/main.c
 *
 * original license
 *   SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *   SPDX-License-Identifier: Unlicense OR CC0-1.0
 *
 */

/* Includes */
#include "bluetooth.h"

#include "common.h"
#include "gap.h"
#include "gatt_svc.h"

QueueHandle_t queueOutSignals = NULL;
QueueHandle_t queueInSignals = NULL;
QueueHandle_t queueOutConfig = NULL;
QueueHandle_t queueInConfig = NULL;
QueueHandle_t queueInAudio = NULL;
QueueHandle_t queueOutAudioList = NULL;

/* Library function declarations */

extern "C" {
    void ble_store_config_init(void);

    /* Private function declarations */
    static void on_stack_reset(int reason);
    static void on_stack_sync(void);
    static void nimble_host_config_init(void);
    static void nimble_host_task(void *param);
}

/* Private functions */
/*
 *  Stack event callback functions
 *      - on_stack_reset is called when host resets BLE stack due to errors
 *      - on_stack_sync is called when host has synced with controller
 */
static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
    /* On stack sync, do advertising initialization */
    adv_init();
}

static void nimble_host_config_init(void) {
    /* Set host callbacks */
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    ble_store_config_init();
}

static void nimble_host_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}


void btStart(void) {

    // setup my queues
    queueOutSignals   = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueInSignals    = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueOutConfig    = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueInConfig     = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueInAudio      = xQueueCreate(1, sizeof(QueueByteBuffer));
    queueOutAudioList = xQueueCreate(1, sizeof(QueueByteBuffer));

    ESP_ERROR_CHECK(
        nimble_port_init());

    ESP_ERROR_CHECK(
        gap_init());

    ESP_ERROR_CHECK(
        gatt_svc_init());

    nimble_host_config_init();

    /* Start NimBLE host task thread and return */
    xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);
    return;
}

void btNotify(void) {
    send_signals_indication();
}

void btStop(void) {
    nimble_port_stop();
}
