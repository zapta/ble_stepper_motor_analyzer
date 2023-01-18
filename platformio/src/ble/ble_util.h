#pragma once

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

namespace ble_util {

const char* gatts_event_name(esp_gatts_cb_event_t event);
const char* gap_ble_event_name(esp_gap_ble_cb_event_t event);

void test_tables();
}  // namespace ble_util