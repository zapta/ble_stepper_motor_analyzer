#pragma once

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

// NOTE: UUID Macros are from 
// https://docs.zephyrproject.org/apidoc/latest/uuid_8h_source.html

// Encoded as 16 hex bytes, LSB first. 
#define ENCODE_UUID_128(w32, w1, w2, w3, w48) \
        (((w48) >>  0) & 0xFF), \
        (((w48) >>  8) & 0xFF), \
        (((w48) >> 16) & 0xFF), \
        (((w48) >> 24) & 0xFF), \
        (((w48) >> 32) & 0xFF), \
        (((w48) >> 40) & 0xFF), \
        (((w3)  >>  0) & 0xFF), \
        (((w3)  >>  8) & 0xFF), \
        (((w2)  >>  0) & 0xFF), \
        (((w2)  >>  8) & 0xFF), \
        (((w1)  >>  0) & 0xFF), \
        (((w1)  >>  8) & 0xFF), \
        (((w32) >>  0) & 0xFF), \
        (((w32) >>  8) & 0xFF), \
        (((w32) >> 16) & 0xFF), \
        (((w32) >> 24) & 0xFF) 

// Encoded as 16 hex bytes, LSB first.
#define ENCODE_UUID_16(w16)  \
        (((w16) >>  0) & 0xFF), \
        (((w16) >>  8) & 0xFF)

namespace ble_util {

const char* gatts_event_name(esp_gatts_cb_event_t event);
const char* gap_ble_event_name(esp_gap_ble_cb_event_t event);

void test_tables();
}  // namespace ble_util