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

#ifndef H_BLEPRPH_
#define H_BLEPRPH_

#include "esp_peripheral.h"
#ifdef __cplusplus
extern "C" {
#endif

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_svr_init(void);

extern uint16_t gatt_svr_chr1_val_handle;  ///< characteristics handle for signals characteristics
extern uint16_t gatt_svr_chr2_val_handle;  ///< characteristics handle for config (proc) characteristics
extern uint16_t gatt_svr_chr3_val_handle;  ///< characteristics handle for audio file characteristics
extern uint16_t gatt_svr_chr4_val_handle;  ///< characteristics handle for audio list characteristics

#ifdef __cplusplus
}
#endif

#endif
