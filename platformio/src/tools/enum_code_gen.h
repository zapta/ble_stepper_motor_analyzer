#pragma once

#include "assert.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"



namespace enum_code_gen {

// class Serializer {
//  public:
//   Serializer(uint8_t* p_start, uint16_t size)
//       : _p_start(p_start), _p_end(p_start + size), _p_next(p_start){};

//   int capacity() { return _p_end - _p_start; }
//   int size() { return _p_next - _p_start; }
//   // bool is_empty() { return _p_next == _p_start; }
//   void reset() { _p_next = _p_start; }

//   inline void append_uint8(uint8_t v) {
//     check_avail(1);
//     *_p_next++ = v;
//   }

//   inline void append_uint16(uint16_t v) {
//     check_avail(2);
//     *_p_next++ = v >> 8;
//     *_p_next++ = v >> 0;
//   }

//   inline void append_int16(int16_t v) { append_uint16((uint16_t)v); }


//   inline void append_uint24(uint32_t v) {
//     check_avail(3);
//     *_p_next++ = v >> 16;
//     *_p_next++ = v >> 8;
//     *_p_next++ = v >> 0;
//   }

//   inline void append_uint32(uint32_t v) {
//     check_avail(4);
//     *_p_next++ = v >> 24;
//     *_p_next++ = v >> 16;
//     *_p_next++ = v >> 8;
//     *_p_next++ = v >> 0;
//   }

//   inline void encode_int32(int32_t v) { append_uint32((uint32_t)v); }

//   inline void append_uint48(uint64_t v) {
//     check_avail(6);
//     *_p_next++ = v >> 40;
//     *_p_next++ = v >> 32;
//     *_p_next++ = v >> 24;
//     *_p_next++ = v >> 16;
//     *_p_next++ = v >> 8;
//     *_p_next++ = v >> 0;
//   }

//  private:
//    uint8_t* const _p_start;
//    uint8_t* const _p_end;
//   uint8_t* _p_next;

//   static constexpr const char* ENCODER_TAG = "msb_enc";

//   inline void check_avail(uint16_t num_bytes) {
//     if ((_p_next + num_bytes) > _p_end) {
//       ESP_LOGE(ENCODER_TAG, "Insufficient space: requested %hu, available %d",
//                num_bytes, (_p_end - _p_next));
//       assert(false);
//     };
//   }
// };

// const char* gatts_event_name(esp_gatts_cb_event_t event);
// const char* gap_ble_event_name(esp_gap_ble_cb_event_t event);
// const char* gatts_status_name(esp_gatt_status_t status );


// // void test_tables();
// // void setup() ;

void gen_tables_code();


}  // namespace enum_code_gen