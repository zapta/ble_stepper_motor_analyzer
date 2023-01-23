#pragma once

#include "assert.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"

// NOTE: UUID Macros are from
// https://docs.zephyrproject.org/apidoc/latest/uuid_8h_source.html

// Encoded as 16 hex bytes, LSB first.
// clang-format off
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
// clang-format on

// Encoded as 16 hex bytes, LSB first.
// clang-format off
#define ENCODE_UUID_16(w16)  \
        (((w16) >>  0) & 0xFF), \
        (((w16) >>  8) & 0xFF)
// clang-format on

namespace ble_util {

// Serialized data to a byte buffer. Big endian order.
class Serializer {
 public:
  Serializer(uint8_t* p_start, uint16_t size)
      : _p_start(p_start), _p_end(p_start + size), _p_next(p_start){};

  int capacity() { return _p_end - _p_start; }
  int size() { return _p_next - _p_start; }
  // bool is_empty() { return _p_next == _p_start; }
  void reset() { _p_next = _p_start; }

  inline void append_uint8(uint8_t v) {
    check_avail(1);
    *_p_next++ = v;
  }

  inline void append_uint16(uint16_t v) {
    check_avail(2);
    *_p_next++ = v >> 8;
    *_p_next++ = v >> 0;
  }

  inline void append_int16(int16_t v) { append_uint16((uint16_t)v); }

  inline void append_uint24(uint32_t v) {
    check_avail(3);
    *_p_next++ = v >> 16;
    *_p_next++ = v >> 8;
    *_p_next++ = v >> 0;
  }

  inline void append_uint32(uint32_t v) {
    check_avail(4);
    *_p_next++ = v >> 24;
    *_p_next++ = v >> 16;
    *_p_next++ = v >> 8;
    *_p_next++ = v >> 0;
  }

  inline void encode_int32(int32_t v) { append_uint32((uint32_t)v); }

  inline void append_uint48(uint64_t v) {
    check_avail(6);
    *_p_next++ = v >> 40;
    *_p_next++ = v >> 32;
    *_p_next++ = v >> 24;
    *_p_next++ = v >> 16;
    *_p_next++ = v >> 8;
    *_p_next++ = v >> 0;
  }

 private:
  uint8_t* const _p_start;
  uint8_t* const _p_end;
  uint8_t* _p_next;

  static constexpr const char* ENCODER_TAG = "msb_enc";

  inline void check_avail(uint16_t num_bytes) {
    if ((_p_next + num_bytes) > _p_end) {
      ESP_LOGE(ENCODER_TAG, "Insufficient space: requested %hu, available %d",
               num_bytes, (_p_end - _p_next));
      assert(false);
    };
  }
};

const char* gatts_event_name(esp_gatts_cb_event_t event);
const char* gap_ble_event_name(esp_gap_ble_cb_event_t event);
const char* gatts_status_name(esp_gatt_status_t status);

}  // namespace ble_util