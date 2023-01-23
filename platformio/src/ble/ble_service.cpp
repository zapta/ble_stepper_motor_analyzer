#include "ble_service.h"

#include <string.h>

#include "acquisition/acq_consts.h"
#include "acquisition/analyzer.h"
#include "ble_util.h"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

// TODO: clean adv_config_done (convert to two booleans)
// TODO: add mutex.

// Based on the sexample at
// https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/bluedroid/ble/gatt_server_service_table/main/gatts_table_creat_demo.c

using gatts_read_evt_param = esp_ble_gatts_cb_param_t::gatts_read_evt_param;
using gatts_write_evt_param = esp_ble_gatts_cb_param_t::gatts_write_evt_param;

namespace ble_service {

static constexpr auto TAG = "ble_srv";

// #define PROFILE_NUM 1
// #define PROFILE_APP_IDX 0
#define ESP_APP_ID 0x55
#define SVC_INST_ID 0

/* The max length of characteristic value. When the GATT client performs a write
 * or prepare write operation, the data length must be less than
 * GATTS_DEMO_CHAR_VAL_LEN_MAX.
 */
// #define GATTS_DEMO_CHAR_VAL_LEN_MAX 500
// #define PREPARE_BUF_MAX_SIZE 1024
// #define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

// TODO: What does this value represent? Is it the correct
// value?
// static constexpr uint16_t kChrDeclSize = sizeof(uint8_t);

// TODO: Replace this with two booleans.
#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)
static uint8_t adv_config_done = 0;

// Invalid connection id (0xffff);
constexpr uint16_t kInvalidConnId = -1;

// bool adv_config_done;
// bool scan_config_done;

// typedef struct {
//   uint8_t *prepare_buf;
//   int prepare_len;
// } prepare_type_env_t;

// static prepare_type_env_t prepare_write_env;

// Bytes are in reversed order.
// static uint8_t service_uuid[16] = {
//     0xd0, 0x4b, 0x8c, 0x01, 0x99, 0x87, 0x30, 0x8a,
//     0x25, 0x45, 0x25, 0x81, 0x34, 0xa0, 0xe1, 0x68,
// };

static const uint8_t service_uuid[] = {
    ENCODE_UUID_128(0x6b6a78d7, 0x8ee0, 0x4a26, 0xba7b, 0x62e357dd9720)};

static const uint8_t model_uuid[] = {ENCODE_UUID_16(0x2a24)};
static const uint8_t revision_uuid[] = {ENCODE_UUID_16(0x2a26)};
static const uint8_t manufacturer_uuid[] = {ENCODE_UUID_16(0x2a29)};
static const uint8_t probe_info_uuid[] = {ENCODE_UUID_16(0xff01)};
static const uint8_t stepper_state_uuid[] = {ENCODE_UUID_16(0xff02)};
static const uint8_t current_histogram_uuid[] = {ENCODE_UUID_16(0xff03)};
static const uint8_t time_histogram_uuid[] = {ENCODE_UUID_16(0xff04)};
static const uint8_t distance_histogram_uuid[] = {ENCODE_UUID_16(0xff05)};

// static const uint16_t kModelChrUuid = ;

// The length of constructed adv and scan respn data must be
// less than 31 bytes. For this reason we split the device
// metadata between the two.
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,  // slave connection min interval, Time =
                             // min_interval * 1.25 msec
    .max_interval = 0x0010,  // slave connection max interval, Time =
                             // max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,        // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL,  // test_manufacturer,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = nullptr,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = false,  // name is passed in the adv packet.
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,        // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL,  //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(service_uuid),
    .p_service_uuid = const_cast<uint8_t *>(service_uuid),
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
#pragma GCC diagnostic pop

struct Vars {
  uint8_t hardware_config;
  uint16_t adc_ticks_per_amp;
  uint16_t gatts_if;
  uint16_t conn_id;  // kInvalidConnId if no connection.
  uint16_t conn_mtu;
  bool notification_enabled;
  analyzer::State stepper_state;
  analyzer::Histogram histogram;
  // Temp buffer for constructing read values.
  // uint8_t value_buffer[300];
  // Used for apps provided responses. Large.
  esp_gatt_rsp_t rsp;
};

static Vars vars = {
    .hardware_config = 0,
    .adc_ticks_per_amp = 0,
    .gatts_if = ESP_GATT_IF_NONE,
    .conn_id = kInvalidConnId,
    .conn_mtu = 0,  //
    .notification_enabled = false,
    .stepper_state = {},
    .histogram = {},
    .rsp = {},
};
// #pragma GCC diagnostic pop

/* Service */
// static const uint16_t GATTS_SERVICE_UUID_TEST = 0x00FF;
static const uint16_t GATTS_CHAR_UUID_TEST_A = 0xFF09;
// static const uint16_t GATTS_CHAR_UUID_TEST_B = 0xFF02;
// static const uint16_t GATTS_CHAR_UUID_TEST_C = 0xFF03;

// static const uint16_t kModelChrUuid = 0xFF03;
// static const uint16_t kModelChrUuid = 0x2A24;
// static const uint16_t kModelChrUuid = 0x2A29;

// static const uint16_t kPrimaryServiceDeclUuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint8_t kPrimaryServiceDeclUuid[] = {
    ENCODE_UUID_16(ESP_GATT_UUID_PRI_SERVICE)};

// static const uint16_t kCharDeclUuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t kCharDeclUuid[] = {
    ENCODE_UUID_16(ESP_GATT_UUID_CHAR_DECLARE)};

// static const uint16_t kChrConfigDeclUuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t kChrConfigDeclUuid[] = {
    ENCODE_UUID_16(ESP_GATT_UUID_CHAR_CLIENT_CONFIG)};

// static const uint16_t character_client_config_uuid =
//     ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

// static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
// static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;

// static const uint8_t kChrPropertyReadWriteNotify =
//     ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ |
//     ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const uint8_t kChrPropertyReadNotify =
    ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const uint8_t kChrPropertyReadOnly = ESP_GATT_CHAR_PROP_BIT_READ;

// TODO: What is this?  Related to notification control.
// TODO: Do we need this?
// static uint8_t heart_measurement_ccc[2] = {0x00, 0x00};

// TODO: Shoudl this be const? Is it nofication status?
static uint8_t state_ccc_val[2] = {0x00, 0x00};

// static const uint8_t char_value[] = {0x33, 0x44, 0x55, 0x66};

static const uint8_t model_str_value[] = "Stepper Probe ESP32";
static constexpr uint16_t model_str_value_len = sizeof(model_str_value) - 1;

static const uint8_t revision_str_value[] = "00.00.01";
static constexpr uint16_t revision_str_value_len =
    sizeof(revision_str_value) - 1;

static const uint8_t manufacturer_str_value[] = "Zapta";
static constexpr uint16_t manufacturer_str_value_len =
    sizeof(manufacturer_str_value) - 1;

// static uint8_t probe_info_value[] = {0x01, 0x02};

// Attributes indexes in tables.
enum {
  ATTR_IDX_SVC,

  ATTR_IDX_MODEL,
  ATTR_IDX_MODEL_VAL,

  ATTR_IDX_REVISION,
  ATTR_IDX_REVISION_VAL,

  ATTR_IDX_MANUFECTURER,
  ATTR_IDX_MANUFECTURER_VAL,

  ATTR_IDX_PROBE_INFO,
  ATTR_IDX_PROBE_INFO_VAL,

  ATTR_IDX_STEPPER_STATE,
  ATTR_IDX_STEPPER_STATE_VAL,
  ATTR_IDX_STEPPER_STATE_CCC,

  ATTR_IDX_CURRENT_HISTOGRAM,
  ATTR_IDX_CURRENT_HISTOGRAM_VAL,

  ATTR_IDX_TIME_HISTOGRAM,
  ATTR_IDX_TIME_HISTOGRAM_VAL,

  ATTR_IDX_DISTANCE_HISTOGRAM,
  ATTR_IDX_DISTANCE_HISTOGRAM_VAL,

  //-------------

  // ATTR_IDX_CHAR_A,
  // ATTR_IDX_CHAR_A_VAL,
  // ATTR_IDX_CHAR_A_CCC,

  // ATTR_IDX_CHAR_B,
  // ATTR_IDX_CHAR_B_VAL,

  // ATTR_IDX_CHAR_C,
  // ATTR_IDX_CHAR_C_VAL,

  ATTR_IDX_COUNT,  // Attributes count.
};

// static uint16_t test_uuid = 0x180a;

// Attributes list of the stepper service. A table like this
// can define a single primary or secondary service.

static const esp_gatts_attr_db_t attr_table[ATTR_IDX_COUNT] = {

    // Service.

    [ATTR_IDX_SVC] = {{ESP_GATT_AUTO_RSP},
                      {ESP_UUID_LEN_16,
                       const_cast<uint8_t *>(kPrimaryServiceDeclUuid),
                       ESP_GATT_PERM_READ, sizeof(service_uuid),
                       sizeof(service_uuid),
                       const_cast<uint8_t *>(service_uuid)}},

    // ----- Device Model.
    //
    // Characteristic
    [ATTR_IDX_MODEL] = {{ESP_GATT_AUTO_RSP},
                        {ESP_UUID_LEN_16, const_cast<uint8_t *>(kCharDeclUuid),
                         ESP_GATT_PERM_READ, sizeof(kChrPropertyReadOnly),
                         sizeof(kChrPropertyReadOnly),
                         const_cast<uint8_t *>(&kChrPropertyReadOnly)}},
    // Value.
    [ATTR_IDX_MODEL_VAL] = {{ESP_GATT_AUTO_RSP},
                            {sizeof(model_uuid),
                             const_cast<uint8_t *>(model_uuid),
                             ESP_GATT_PERM_READ, model_str_value_len,
                             model_str_value_len,
                             const_cast<uint8_t *>(model_str_value)}},

    // ----- Firmware revision
    //
    // Characteristic
    [ATTR_IDX_REVISION] = {{ESP_GATT_AUTO_RSP},
                           {ESP_UUID_LEN_16,
                            const_cast<uint8_t *>(kCharDeclUuid),
                            ESP_GATT_PERM_READ, sizeof(kChrPropertyReadOnly),
                            sizeof(kChrPropertyReadOnly),
                            (uint8_t *)&kChrPropertyReadOnly}},
    // Value.
    [ATTR_IDX_REVISION_VAL] = {{ESP_GATT_AUTO_RSP},
                               {sizeof(revision_uuid),
                                const_cast<uint8_t *>(revision_uuid),
                                ESP_GATT_PERM_READ, revision_str_value_len,
                                revision_str_value_len,
                                const_cast<uint8_t *>(revision_str_value)}},

    // ----- Manufacturer name
    //
    // Characteristic
    [ATTR_IDX_MANUFECTURER] =
        {{ESP_GATT_AUTO_RSP},
         {ESP_UUID_LEN_16, const_cast<uint8_t *>(kCharDeclUuid),
          ESP_GATT_PERM_READ, sizeof(kChrPropertyReadOnly),
          sizeof(kChrPropertyReadOnly), (uint8_t *)&kChrPropertyReadOnly}},
    // Value.
    [ATTR_IDX_MANUFECTURER_VAL] =
        {{ESP_GATT_AUTO_RSP},
         {sizeof(manufacturer_uuid), const_cast<uint8_t *>(manufacturer_uuid),
          ESP_GATT_PERM_READ, manufacturer_str_value_len,
          manufacturer_str_value_len,
          const_cast<uint8_t *>(manufacturer_str_value)}},

    // ----- Probe info
    //
    // Characteristic
    [ATTR_IDX_PROBE_INFO] = {{ESP_GATT_AUTO_RSP},
                             {ESP_UUID_LEN_16,
                              const_cast<uint8_t *>(kCharDeclUuid),
                              ESP_GATT_PERM_READ, sizeof(kChrPropertyReadOnly),
                              sizeof(kChrPropertyReadOnly),
                              const_cast<uint8_t *>(&kChrPropertyReadOnly)}},
    // Value
    [ATTR_IDX_PROBE_INFO_VAL] = {{ESP_GATT_RSP_BY_APP},
                                 {sizeof(probe_info_uuid),
                                  const_cast<uint8_t *>(probe_info_uuid),
                                  ESP_GATT_PERM_READ, 0, 0, nullptr}},

    // ----- Stepper state.
    //
    // Characteristic
    //@@@
    [ATTR_IDX_STEPPER_STATE] =
        {{ESP_GATT_AUTO_RSP},
         {ESP_UUID_LEN_16, const_cast<uint8_t *>(kCharDeclUuid),
          ESP_GATT_PERM_READ, sizeof(kChrPropertyReadNotify),
          sizeof(kChrPropertyReadNotify),
          const_cast<uint8_t *>(&kChrPropertyReadNotify)}},
    // Value
    [ATTR_IDX_STEPPER_STATE_VAL] = {{ESP_GATT_RSP_BY_APP},
                                    {sizeof(stepper_state_uuid),
                                     const_cast<uint8_t *>(stepper_state_uuid),
                                     ESP_GATT_PERM_READ, 0, 0, nullptr}},

    // Client Characteristic Configuration Descriptor
    [ATTR_IDX_STEPPER_STATE_CCC] = {{ESP_GATT_AUTO_RSP},
                                    {ESP_UUID_LEN_16,
                                     (uint8_t *)&kChrConfigDeclUuid,
                                     ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                     sizeof(state_ccc_val),
                                     sizeof(state_ccc_val), state_ccc_val}},

    // ----- Current histogram.
    //
    // Characteristic
    [ATTR_IDX_CURRENT_HISTOGRAM] =
        {{ESP_GATT_AUTO_RSP},
         {ESP_UUID_LEN_16, const_cast<uint8_t *>(kCharDeclUuid),
          ESP_GATT_PERM_READ, sizeof(kChrPropertyReadOnly),
          sizeof(kChrPropertyReadOnly),
          const_cast<uint8_t *>(&kChrPropertyReadOnly)}},
    // Value
    [ATTR_IDX_CURRENT_HISTOGRAM_VAL] = {{ESP_GATT_RSP_BY_APP},
                                        {sizeof(current_histogram_uuid),
                                         const_cast<uint8_t *>(
                                             current_histogram_uuid),
                                         ESP_GATT_PERM_READ, 0, 0, nullptr}},

    // ----- Time histogram.
    //
    // Characteristic
    [ATTR_IDX_TIME_HISTOGRAM] =
        {{ESP_GATT_AUTO_RSP},
         {ESP_UUID_LEN_16, const_cast<uint8_t *>(kCharDeclUuid),
          ESP_GATT_PERM_READ, sizeof(kChrPropertyReadOnly),
          sizeof(kChrPropertyReadOnly),
          const_cast<uint8_t *>(&kChrPropertyReadOnly)}},
    // Value
    [ATTR_IDX_TIME_HISTOGRAM_VAL] = {{ESP_GATT_RSP_BY_APP},
                                     {sizeof(time_histogram_uuid),
                                      const_cast<uint8_t *>(
                                          time_histogram_uuid),
                                      ESP_GATT_PERM_READ, 0, 0, nullptr}},

    // ----- Distance histogram.
    //
    // Characteristic
    [ATTR_IDX_DISTANCE_HISTOGRAM] =
        {{ESP_GATT_AUTO_RSP},
         {ESP_UUID_LEN_16, const_cast<uint8_t *>(kCharDeclUuid),
          ESP_GATT_PERM_READ, sizeof(kChrPropertyReadOnly),
          sizeof(kChrPropertyReadOnly),
          const_cast<uint8_t *>(&kChrPropertyReadOnly)}},
    // Value
    [ATTR_IDX_DISTANCE_HISTOGRAM_VAL] = {{ESP_GATT_RSP_BY_APP},
                                         {sizeof(distance_histogram_uuid),
                                          const_cast<uint8_t *>(
                                              distance_histogram_uuid),
                                          ESP_GATT_PERM_READ, 0, 0, nullptr}},

    // ----- XYZ charateristic.

    // Characteristic Declaration
    // [ATTR_IDX_CHAR_A] = {{ESP_GATT_AUTO_RSP},
    //                      {ESP_UUID_LEN_16, const_cast<uint8_t
    //                      *>(kCharDeclUuid),
    //                       ESP_GATT_PERM_READ,
    //                       sizeof(kChrPropertyReadWriteNotify),
    //                       sizeof(kChrPropertyReadWriteNotify),
    //                       (uint8_t *)&kChrPropertyReadWriteNotify}},

    // // Characteristic Value
    // [ATTR_IDX_CHAR_A_VAL] = {{ESP_GATT_AUTO_RSP},
    //                          {ESP_UUID_LEN_16,
    //                           (uint8_t *)&GATTS_CHAR_UUID_TEST_A,
    //                           ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    //                           GATTS_DEMO_CHAR_VAL_LEN_MAX,
    //                           sizeof(char_value),
    //                           (uint8_t *)&char_value}},

    // // Client Characteristic Configuration Descriptor
    // [ATTR_IDX_CHAR_A_CCC] = {{ESP_GATT_AUTO_RSP},
    //                          {ESP_UUID_LEN_16, (uint8_t
    //                          *)&kChrConfigDeclUuid,
    //                           ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    //                           sizeof(uint16_t),
    //                           sizeof(heart_measurement_ccc),
    //                           heart_measurement_ccc}},

};

// Parallel to the entries of attr_table.
uint16_t handle_table[ATTR_IDX_COUNT];

// For read events only.
// static void send_read_error_response(esp_gatt_if_t gatts_if,
//                                      const gatts_read_evt_param &read_param,
//                                      esp_gatt_status_t gatt_error) {
//   // TODO: This is a large variable. Do we need to clear entirely?
//   memset(&vars.rsp, 0, sizeof(vars.rsp));
//   vars.rsp.attr_value.handle = read_param.handle;
//   vars.rsp.attr_value.len = 0;
//   esp_ble_gatts_send_response(gatts_if, read_param.conn_id,
//   read_param.trans_id,
//                               gatt_error, &vars.rsp);
// }

static esp_gatt_status_t on_probe_info_read(
    const gatts_read_evt_param &read_param, ble_util::Serializer *ser) {
  ESP_LOGI(TAG, "on_probe_info_read() called");

  // TODO: This is a large variable. Do we need to clear entirely?
  // memset(&vars.rsp, 0, sizeof(vars.rsp));

  // Serialize the response.
  // ble_util::Serializer enc(vars.rsp.attr_value.value,
  //                          sizeof(vars.rsp.attr_value.value));

  assert(ser->size() == 0);
  ser->append_uint8(0x1);  // Packet format version
  ser->append_uint8(vars.hardware_config);
  ser->append_uint16(vars.adc_ticks_per_amp);
  ser->append_uint24(acq_consts::kTimeTicksPerSec);
  ser->append_uint16(acq_consts::kBucketStepsPerSecond);
  assert(ser->size() == 9);

  // vars.rsp.attr_value.len = enc.size();
  // vars.rsp.attr_value.handle = read_param.handle;
  // esp_ble_gatts_send_response(gatts_if, read_param.conn_id,
  // read_param.trans_id,
  //                             ESP_GATT_OK, &vars.rsp);
  // ESP_LOGI(TAG, "on_probe_info_read() sent response with %d bytes",
  //          ser->size());
  return ESP_GATT_OK;
}

static void serialize_state(const analyzer::State &state,
                            ble_util::Serializer *ser) {
  // Flags.
  // * bit5 : true IFF energized.
  // * bit4 : true IFF reversed direction.
  // * bit1 : quadrant MSB.
  // * bit0 : quadrant LSB.
  //
  // Quarant is in the range [0, 3].
  // All other bits are reserved and readers should treat them
  // as undefined.
  const uint8_t flags = (state.is_energized ? 0x20 : 0) |
                        (state.is_reverse_direction ? 0x10 : 0) |
                        (state.quadrant & 0x03);

  assert(ser->size() == 0);
  ser->append_uint48(state.tick_count);
  ser->encode_int32(state.full_steps);
  ser->append_uint8(flags);
  ser->append_int16(state.v1);
  ser->append_int16(state.v2);
  ser->append_uint32(state.non_energized_count);
  assert(ser->size() == 19);
}

static esp_gatt_status_t on_stepper_state_read(
    const gatts_read_evt_param &read_param, ble_util::Serializer *ser) {
  ESP_LOGI(TAG, "on_stepper_state_read() called");

  // ESP_LOG_BUFFER_HEX(TAG, state_ccc_val, sizeof(state_ccc_val));

  // TODO: This is a large variable. Do we need to clear entirely?
  // memset(&vars.rsp, 0, sizeof(vars.rsp));

  analyzer::sample_state(&vars.stepper_state);

  // Encode packet the response.
  // ble_util::Serializer enc(vars.rsp.attr_value.value,
  //                          sizeof(vars.rsp.attr_value.value));
  serialize_state(vars.stepper_state, ser);
  // Flags.
  // * bit5 : true IFF energized.
  // * bit4 : true IFF reversed direction.
  // * bit1 : quadrant MSB.
  // * bit0 : quadrant LSB.
  //
  // Quarant is in the range [0, 3].
  // All other bits are reserved and readers should treat them
  // as undefined.
  // const uint8_t flags = (state.is_energized ? 0x20 : 0) |
  //                       (state.is_reverse_direction ? 0x10 : 0) |
  //                       (state.quadrant & 0x03);

  // enc.encode_uint48(state.tick_count);
  // enc.encode_int32(state.full_steps);
  // enc.encode_uint8(flags);
  // enc.encode_int16(state.v1);
  // enc.encode_int16(state.v2);
  // enc.encode_uint32(state.non_energized_count);
  // assert(enc.size() == 19);

  // vars.rsp.attr_value.len = enc.size();
  // vars.rsp.attr_value.handle = read_param.handle;
  // esp_ble_gatts_send_response(gatts_if, read_param.conn_id,
  // read_param.trans_id,
  //                             ESP_GATT_OK, &vars.rsp);
  // ESP_LOGI(TAG, "on_stepper_state_read() sent response with %d bytes",
  //          ser->size());

  return ESP_GATT_OK;
}

static esp_gatt_status_t on_current_histogram_read(
    const gatts_read_evt_param &read_param, ble_util::Serializer *ser) {
  ESP_LOGI(TAG, "on_current_histogram_read() called");

  analyzer::sample_histogram(&vars.histogram);

  assert(ser->size() == 0);
  ser->append_uint8(0x10);  // format id.

  ser->append_uint8(acq_consts::kNumHistogramBuckets);  // Num of buckets

  // Format bucket values, (2 bytes each)
  for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
    const analyzer::HistogramBucket &bucket = vars.histogram.buckets[i];
    uint16_t value;
    if (bucket.total_steps == 0) {
      value = 0;
    } else {
      value = bucket.total_step_peak_currents / bucket.total_steps;
      if (value == 0) {
        // We use 0 to indicate zero steps.
        value = 1;
      }
    }
    ser->append_uint16(value);
  }

  return ESP_GATT_OK;
}

static esp_gatt_status_t on_time_histogram_read(
    const gatts_read_evt_param &read_param, ble_util::Serializer *ser) {
  ESP_LOGI(TAG, "on_time_histogram_read() called");

  analyzer::sample_histogram(&vars.histogram);

  assert(ser->size() == 0);
  ser->append_uint8(0x20);                              // format id.
  ser->append_uint8(acq_consts::kNumHistogramBuckets);  // Num of buckets

  // Find total time value.
  uint64_t total_ticks = 0;
  for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
    total_ticks += vars.histogram.buckets[i].total_ticks_in_steps;
  }

  if (total_ticks < 10) {
    // Special case: When the total time is low, we just set the entire
    // histogram as zero. This also prevents divide by zero.
    for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
      ser->append_uint16(0);
    }
  } else {
    // Normal case: encide relative values as permils (0.1%) of the
    // totoal time. [0, 1000]
    for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
      const uint16_t normalized_val =
          (vars.histogram.buckets[i].total_ticks_in_steps * 1000) / total_ticks;
      ser->append_uint16(normalized_val);
    }
  }

  return ESP_GATT_OK;
}

static esp_gatt_status_t on_distance_histogram_read(
    const gatts_read_evt_param &read_param, ble_util::Serializer *ser) {
  ESP_LOGI(TAG, "on_distance_histogram_read() called");

  analyzer::sample_histogram(&vars.histogram);

  assert(ser->size() == 0);

  // Encode the result value.
  // uint8_t *const p0 = static_cast<uint8_t *>(buf);
  // uint8_t *p = p0;

  ser->append_uint8(0x30);  // Format id.
  // Format id (1 byte)
  // *p++ = 0x30;  // Format id.

  ser->append_uint8(acq_consts::kNumHistogramBuckets);  // Num buckets
  // Num points: (1 byte)
  // *p++ = acq_consts::kNumHistogramBuckets;

  // Find max distance value
  uint64_t total_steps = 0;
  for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
    // if (histogram_snapshot.buckets[i].total_steps > max_value) {
    total_steps += vars.histogram.buckets[i].total_steps;
    // }
  }

  if (total_steps < 10) {
    // Special case: When the total time is low, we just set the entire
    // histogram as zero. This also prevents divide by zero.
    for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
      ser->append_uint16(0);
      // *p++ = 0;
      // *p++ = 0;
    }
  } else {
    // Normal case: encide relative values as permils (0.1%) of the
    // total. [0, 1000].
    for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
      const uint16_t normalized_val =
          (vars.histogram.buckets[i].total_steps * 1000) / total_steps;
      ser->append_uint16(normalized_val);
      // *p++ = normalized_val >> 8;
      // *p++ = normalized_val;
      // printk("%d: %llu\n", i, histogram_sample.buckets[i].total_steps);
    }
  }

  return ESP_GATT_OK;
}

static esp_gatt_status_t on_notification_control_write(
    const gatts_write_evt_param &write_param) {
  if (write_param.len != 2 || write_param.is_prep) {
    return ESP_GATT_ERROR;
  }

  const uint16_t descr_value = write_param.value[1] << 8 | write_param.value[0];
  const bool notification_enabled = descr_value & 0x0001;
  ESP_LOGW(TAG, "Notifications 0x%04x: %d -> %d", descr_value,
           vars.notification_enabled, notification_enabled);
  vars.notification_enabled = notification_enabled;

  return ESP_GATT_OK;

  // if (write_param.need_rsp) {
  //   esp_ble_gatts_send_response(gatts_if, write_param.conn_id,
  //                               write_param.trans_id, status, NULL);
  // }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
      adv_config_done &= (~ADV_CONFIG_FLAG);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;

    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
      adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;

      // #endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      /* advertising start complete event to indicate advertising start
       * successfully or failed */
      if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "advertising start failed");
      } else {
        ESP_LOGI(TAG, "advertising start successfully");
      }
      break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
      if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Advertising stop failed");
      } else {
        ESP_LOGI(TAG, "Stop adv successfully\n");
      }
      break;

    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
      ESP_LOGI(
          TAG,
          "update connection params status = %d, min_int = %d, max_int = "
          "%d,conn_int = %d,latency = %d, timeout = %d",
          param->update_conn_params.status, param->update_conn_params.min_int,
          param->update_conn_params.max_int, param->update_conn_params.conn_int,
          param->update_conn_params.latency, param->update_conn_params.timeout);
      break;

    default:
      ESP_LOGI(TAG, "Gap event handler: unknown event %d, %s", event,
               ble_util::gap_ble_event_name(event));
      break;
  }
}

// void example_prepare_write_event_env(esp_gatt_if_t gatts_if,
//                                      prepare_type_env_t *prepare_write_env,
//                                      const esp_ble_gatts_cb_param_t *param) {
//   ESP_LOGI(TAG, "prepare write, handle = %d, value len = %d",
//            param->write.handle, param->write.len);
//   esp_gatt_status_t status = ESP_GATT_OK;
//   if (prepare_write_env->prepare_buf == NULL) {
//     prepare_write_env->prepare_buf =
//         (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
//     prepare_write_env->prepare_len = 0;
//     if (prepare_write_env->prepare_buf == NULL) {
//       ESP_LOGE(TAG, "%s, Gatt_server prep no mem", __func__);
//       status = ESP_GATT_NO_RESOURCES;
//     }
//   } else {
//     if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
//       status = ESP_GATT_INVALID_OFFSET;
//     } else if ((param->write.offset + param->write.len) >
//                PREPARE_BUF_MAX_SIZE) {
//       status = ESP_GATT_INVALID_ATTR_LEN;
//     }
//   }
//   /*send response when param->write.need_rsp is true */
//   if (param->write.need_rsp) {
//     esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t
//     *)malloc(sizeof(esp_gatt_rsp_t)); if (gatt_rsp != NULL) {
//       gatt_rsp->attr_value.len = param->write.len;
//       gatt_rsp->attr_value.handle = param->write.handle;
//       gatt_rsp->attr_value.offset = param->write.offset;
//       gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
//       memcpy(gatt_rsp->attr_value.value, param->write.value,
//       param->write.len); esp_err_t response_err =
//           esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
//                                       param->write.trans_id, status,
//                                       gatt_rsp);
//       if (response_err != ESP_OK) {
//         ESP_LOGE(TAG, "Send response error");
//       }
//       free(gatt_rsp);
//     } else {
//       ESP_LOGE(TAG, "%s, malloc failed", __func__);
//     }
//   }
//   if (status != ESP_GATT_OK) {
//     return;
//   }
//   memcpy(prepare_write_env->prepare_buf + param->write.offset,
//          param->write.value, param->write.len);
//   prepare_write_env->prepare_len += param->write.len;
// }

// void example_exec_write_event_env(prepare_type_env_t *prepare_write_env,
//                                   const esp_ble_gatts_cb_param_t *param) {
//   if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC &&
//       prepare_write_env->prepare_buf) {
//     esp_log_buffer_hex(TAG, prepare_write_env->prepare_buf,
//                        prepare_write_env->prepare_len);
//   } else {
//     ESP_LOGI(TAG, "ESP_GATT_PREP_WRITE_CANCEL");
//   }
//   if (prepare_write_env->prepare_buf) {
//     free(prepare_write_env->prepare_buf);
//     prepare_write_env->prepare_buf = NULL;
//   }
//   prepare_write_env->prepare_len = 0;
// }

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param) {
  switch (event) {
    case ESP_GATTS_REG_EVT: {
      assert(param->reg.status == ESP_GATT_OK);
      vars.gatts_if = gatts_if;

      // @@@ From other handler.
      // if (param->reg.status == ESP_GATT_OK) {
      //   // profile_table[PROFILE_APP_IDX].gatts_if = gatts_if;
      //   profile.gatts_if = gatts_if;
      // } else {
      //   ESP_LOGE(TAG, "reg app failed, app_id %04x, status %d",
      //            param->reg.app_id, param->reg.status);
      //   return;
      // }

      // @@@ From this handler.
      // Construct device name from device address.
      const uint8_t *device_addr = esp_bt_dev_get_address();
      assert(device_addr);
      char device_name[20];
      snprintf(device_name, sizeof(device_name), "STP-%02X%02X%02X%02X%02X%02X",
               device_addr[0], device_addr[1], device_addr[2], device_addr[3],
               device_addr[4], device_addr[5]);
      // NOTE: Device name is copied internally so its safe to pass a temporary
      // name pointer
      ESP_LOGI(TAG, "Device name: %s", device_name);
      //   esp_bt_dev_set_device_name(device_name);
      esp_err_t err = esp_ble_gap_set_device_name(device_name);
      if (err) {
        ESP_LOGE(TAG, "set device name failed, error code = %x", err);
      }

      // config adv data
      err = esp_ble_gap_config_adv_data(&adv_data);
      if (err) {
        ESP_LOGE(TAG, "config adv data failed, error code = %x", err);
      }
      adv_config_done |= ADV_CONFIG_FLAG;

      // config scan response data
      err = esp_ble_gap_config_adv_data(&scan_rsp_data);
      if (err) {
        ESP_LOGE(TAG, "config scan response data failed, error code = %x", err);
      }
      adv_config_done |= SCAN_RSP_CONFIG_FLAG;

      err = esp_ble_gatts_create_attr_tab(attr_table, gatts_if, ATTR_IDX_COUNT,
                                          SVC_INST_ID);
      if (err) {
        ESP_LOGE(TAG, "create attr table failed, error code = %x", err);
      }
    } break;

    // Handle read event.
    case ESP_GATTS_READ_EVT: {
      ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
      const gatts_read_evt_param &read_param = param->read;

      // For characteristics with auto resp we don't need to do
      // anything here.
      if (!read_param.need_rsp) {
        ESP_LOGI(TAG, "ESP_GATTS_READ_EVT, rsp not needed.");
        return;
      }

      // Here when a read with a user constructed response. In our
      // // app all of those reads expect offset == 0.
      // if (read_param.offset != 0) {
      //   ESP_LOGE(TAG, "Read request has an invalid offset: %hu",
      //            read_param.offset);
      //   send_read_error_response(gatts_if, read_param,
      //   ESP_GATT_INVALID_OFFSET); return;
      // }

      // TODO: This is a large variable. Do we need to clear entirely?
      memset(&vars.rsp, 0, sizeof(vars.rsp));

      ble_util::Serializer ser(vars.rsp.attr_value.value,
                               sizeof(vars.rsp.attr_value.value));
      esp_gatt_status_t status = ESP_GATT_NOT_FOUND;

      if (read_param.offset != 0) {
        status = ESP_GATT_INVALID_OFFSET;
      } else if (read_param.handle == handle_table[ATTR_IDX_PROBE_INFO_VAL]) {
        status = on_probe_info_read(read_param, &ser);
      } else if (read_param.handle ==
                 handle_table[ATTR_IDX_STEPPER_STATE_VAL]) {
        status = on_stepper_state_read(read_param, &ser);
      } else if (read_param.handle ==
                 handle_table[ATTR_IDX_CURRENT_HISTOGRAM_VAL]) {
        status = on_current_histogram_read(read_param, &ser);
      } else if (read_param.handle ==
                 handle_table[ATTR_IDX_TIME_HISTOGRAM_VAL]) {
        status = on_time_histogram_read(read_param, &ser);
      } else if (read_param.handle ==
                 handle_table[ATTR_IDX_DISTANCE_HISTOGRAM_VAL]) {
        status = on_distance_histogram_read(read_param, &ser);
      }

      const uint16_t len = (status == ESP_GATT_OK) ? ser.size() : 0;
      ESP_LOGE(TAG, "ESP_GATTS_READ_EVT: response status: %hu %s, len: %hu",
               status, ble_util::gatts_status_name(status), len);
      vars.rsp.attr_value.len = len;
      vars.rsp.attr_value.handle = read_param.handle;
      esp_ble_gatts_send_response(gatts_if, read_param.conn_id,
                                  read_param.trans_id, status, &vars.rsp);

    } break;

    // Write event
    case ESP_GATTS_WRITE_EVT: {
      const gatts_write_evt_param &write_param = param->write;
      ESP_LOGW(TAG,
               "ESP_GATTS_WRITE_EVT event: handle=%d, is_prep=%d, need_rsp=%d, "
               "len=%d, value:",
               write_param.handle, write_param.is_prep, write_param.need_rsp,
               write_param.len);
      esp_log_buffer_hex(TAG, write_param.value, write_param.len);

      if (write_param.is_prep) {
        ESP_LOGW(TAG, "Unexpected is_prep write");
        esp_ble_gatts_send_response(gatts_if, write_param.conn_id,
                                    write_param.trans_id, ESP_GATT_OK, NULL);
      }

      esp_gatt_status_t status = ESP_GATT_OK;

      // Write to notification control.
      if (handle_table[ATTR_IDX_STEPPER_STATE_CCC] == write_param.handle) {
        status = on_notification_control_write(write_param);
        // esp_gatt_status_t status = ESP_GATT_OK;

        // if (write_param.len != 2 || write_param.is_prep) {
        //   status = ESP_GATT_ERROR;
        // } else {
        //   const uint16_t descr_value =
        //       write_param.value[1] << 8 | write_param.value[0];
        //   const bool notification_enabled = descr_value & 0x0001;
        //   ESP_LOGW(TAG, "Notifications 0x%04x: %d -> %d", descr_value,
        //            vars.notification_enabled, notification_enabled);
        //   vars.notification_enabled = notification_enabled;
        // }
        // if (write_param.need_rsp) {
        //   esp_ble_gatts_send_response(gatts_if, write_param.conn_id,
        //                               write_param.trans_id, status, NULL);
        // }
        // break;
      }

      // Send request is requested.
      if (write_param.need_rsp) {
        ESP_LOGW(TAG, "Sending write response with status %d %s", status,
                 ble_util::gatts_status_name(status));
        esp_ble_gatts_send_response(gatts_if, write_param.conn_id,
                                    write_param.trans_id, status, NULL);
      } else {
        ESP_LOGW(TAG, "Write response not requested");
      }
    } break;

      // case ESP_GATTS_EXEC_WRITE_EVT:
      //   // the length of gattc prepare write data must be less than
      //   // GATTS_DEMO_CHAR_VAL_LEN_MAX.
      //   ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
      //   // example_exec_write_event_env(&prepare_write_env, param);
      //   break;

    case ESP_GATTS_MTU_EVT:
      ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
      vars.conn_mtu = param->mtu.mtu;
      break;

    case ESP_GATTS_START_EVT:
      ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d",
               param->start.status, param->start.service_handle);
      break;

    case ESP_GATTS_CONNECT_EVT: {
      ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d",
               param->connect.conn_id);
      // Verify our assumption that kInvalidConnId can't be a valid id.
      assert(param->connect.conn_id != kInvalidConnId);
      vars.conn_id = param->connect.conn_id;
      vars.conn_mtu = 23;  // Initial BLE MTU.
      esp_log_buffer_hex(TAG, param->connect.remote_bda, 6);
      esp_ble_conn_update_params_t conn_params = {};
      memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
      /* For the iOS system, please refer to Apple official documents about
       * the BLE connection parameters restrictions. */
      conn_params.latency = 0;
      conn_params.max_int = 0x20;  // max_int = 0x20*1.25ms = 40ms
      conn_params.min_int = 0x10;  // min_int = 0x10*1.25ms = 20ms
      conn_params.timeout = 400;   // timeout = 400*10ms = 4000ms
      // start sent the update connection parameters to the peer device.
      esp_ble_gap_update_conn_params(&conn_params);
    } break;

    // On connection disconnection.
    case ESP_GATTS_DISCONNECT_EVT:
      ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x",
               param->disconnect.reason);
      vars.conn_id = kInvalidConnId;
      vars.conn_mtu = 0;
      vars.notification_enabled = false;
      esp_ble_gap_start_advertising(&adv_params);
      break;

    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
      if (param->add_attr_tab.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "create attribute table failed, error code=0x%x",
                 param->add_attr_tab.status);
      } else if (param->add_attr_tab.num_handle != ATTR_IDX_COUNT) {
        ESP_LOGE(TAG,
                 "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)",
                 param->add_attr_tab.num_handle, ATTR_IDX_COUNT);
      } else {
        ESP_LOGI(TAG, "create attribute table successfully, num handles = %d",
                 param->add_attr_tab.num_handle);
        memcpy(handle_table, param->add_attr_tab.handles, sizeof(handle_table));
        esp_ble_gatts_start_service(handle_table[ATTR_IDX_SVC]);
        // esp_ble_gatts_start_service(handle_table[ATTR_IDX_SVC]);
      }
      break;
    }

    // TODO: explain here when this is called. Is it just for
    // app provided responses?
    case ESP_GATTS_RESPONSE_EVT:
      if (param->rsp.status == ESP_GATT_OK)
        ESP_LOGI(TAG, "ESP_GATTS_RESPONSE_EVT rsp OK");
      else {
        ESP_LOGW(TAG, "ESP_GATTS_RESPONSE_EVT rsp error: %d",
                 param->rsp.status);
      }
      break;

    // Ignored sliently.
    case ESP_GATTS_CONF_EVT:
      break;

    // Unexpected, so logging it.
    default:
      ESP_LOGI(TAG, "Gatt event handler: ignored event %d, %s", event,
               ble_util::gatts_event_name(event));
      break;
  }
}

void setup(uint8_t hardware_config, uint16_t adc_ticks_per_amp) {
  // ble_util::setup();
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  vars.hardware_config = hardware_config;
  vars.adc_ticks_per_amp = adc_ticks_per_amp;

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  // NOTE: Non default bg_cfg values can be set here.
  esp_err_t ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(TAG, "%s enable controller failed: %s", __func__,
             esp_err_to_name(ret));
    assert(0);
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(TAG, "%s enable controller failed: %s", __func__,
             esp_err_to_name(ret));
    assert(0);
  }

  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__,
             esp_err_to_name(ret));
    assert(0);
  }

  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__,
             esp_err_to_name(ret));
    assert(0);
  }

  ret = esp_ble_gatts_register_callback(gatts_event_handler);
  if (ret) {
    ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
    assert(0);
  }

  ret = esp_ble_gap_register_callback(gap_event_handler);
  if (ret) {
    ESP_LOGE(TAG, "gap register error, error code = %x", ret);
    assert(0);
  }

  ret = esp_ble_gatts_app_register(ESP_APP_ID);
  if (ret) {
    ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
    assert(0);
  }

  // Set MTU to max of 247 which means max payload data
  // can be 247-3 = 244 bytes long.
  // https://discord.com/channels/720317445772017664/733036837576376341/971235966973018193
  // in payload size of 247-3 = 244. However, this is negotiated with
  // the client so can be in practice lower that this max.
  ret = esp_ble_gatt_set_local_mtu(247);
  if (ret) {
    ESP_LOGE(TAG, "set local  MTU failed, error code = %x", ret);
    assert(0);
  }
}

static uint8_t notification_buffer[50] = {};
// static uint32_t notify_count = 0;

void notify_state_if_enabled(const analyzer::State &state) {

  // esp_log_buffer_hex(TAG, state_ccc_val,  sizeof(state_ccc_val));

  // ESP_LOG_BUFFER_HEX(TAG, state_ccc_val, sizeof(state_ccc_val));
  //  ESP_LOG_BUFFER_HEX(TAG, heart_measurement_ccc,
  //  sizeof(heart_measurement_ccc));

  // const bool notification_enabled = state_ccc_val[0] & 0x01;
  if (!vars.notification_enabled) {
    return;
  }

  assert(vars.gatts_if != ESP_GATT_IF_NONE);
  assert(vars.conn_id != kInvalidConnId);

  // notify_count++;
  // ESP_LOGI(TAG, "Sending a notification...");

  ble_util::Serializer ser(notification_buffer, sizeof(notification_buffer));
  serialize_state(state, &ser);

  // notify_data[0] = (uint8_t)(notify_count >> 8);
  // notify_data[1] = (uint8_t)notify_count;

  // const gatts_profile_inst &profile = profile_table[PROFILE_APP_IDX];

  // Having need_config == false, means notification, not indicate.
  const esp_err_t err = esp_ble_gatts_send_indicate(
      vars.gatts_if, vars.conn_id, handle_table[ATTR_IDX_STEPPER_STATE_VAL],
      ser.size(), notification_buffer, false);

  if (err) {
    ESP_LOGE(TAG, "esp_ble_gatts_send_indicate() returned err %d", err);
  }

  // ESP_LOGI(TAG, "Notification sent.");
}

}  // namespace ble_service