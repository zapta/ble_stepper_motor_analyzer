#include "ble_host.h"

#include <algorithm>
#include <string.h>

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
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "acquisition/acq_consts.h"
#include "acquisition/analyzer.h"
#include "ble_util.h"
#include "misc/util.h"
#include "settings/controls.h"

// Based on the sexample at
// https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/bluedroid/ble/gatt_server_service_table/main/gatts_table_creat_demo.c

using gatts_read_evt_param = esp_ble_gatts_cb_param_t::gatts_read_evt_param;
using gatts_write_evt_param = esp_ble_gatts_cb_param_t::gatts_write_evt_param;

namespace ble_host {

static constexpr auto TAG = "ble_host";

#define ESP_APP_ID 0x55
#define SVC_INST_ID 0

static SemaphoreHandle_t protected_vars_mutex;
#define ENTER_MUTEX \
  { xSemaphoreTake(protected_vars_mutex, portMAX_DELAY); }
#define EXIT_MUTEX \
  { xSemaphoreGive(protected_vars_mutex); }

// Set MTU to max of 247 which means max payload data
// can be 247-3 = 244 bytes long.
// https://discord.com/channels/720317445772017664/733036837576376341/971235966973018193
// in payload size of 247-3 = 244. However, this is negotiated with
// the client so can be in practice lower that this max.
static constexpr uint16_t kMaxRequestedMtu = 247;
static constexpr uint16_t kMtuOverhead = 3;

// Invalid connection id (0xffff);
constexpr uint16_t kInvalidConnId = -1;

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
static const uint8_t command_uuid[] = {ENCODE_UUID_16(0xff06)};
static const uint8_t capture_uuid[] = {ENCODE_UUID_16(0xff07)};

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
    .manufacturer_len = 0,
    .p_manufacturer_data = nullptr,
    .service_data_len = 0,
    .p_service_data = nullptr,
    .service_uuid_len = 0,
    .p_service_uuid = nullptr,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = false,  // name is passed in the adv packet.
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    // Manufacturer len/data is set later with device nickname.
    .manufacturer_len = 0,
    .p_manufacturer_data = nullptr,
    .service_data_len = 0,
    .p_service_data = nullptr,
    .service_uuid_len = 0,
    .p_service_uuid = nullptr,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,  // x 0.625ms = 20ms
    .adv_int_max = 0x40,  // x 0.625ms = 40ms
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr = {0},
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// These vars accessed ony from the BLE callback thread and
// thus don't need mutex protection.
struct Vars {
  uint8_t hardware_config = 0;
  bool adv_data_configured = false;
  bool scan_rsp_configured = false;
  uint16_t adc_ticks_per_amp = 0;
  // Contains the advertised manufacturer data with the device nickname.
  // This is set later at runtime
  uint8_t manufacturer_data[20] = {0};
  uint16_t manufacturer_data_len = 0;
  uint16_t conn_mtu = 0;
  analyzer::State stepper_state_buffer = {};
  analyzer::Histogram histogram_buffer = {};
  // Number of capture points already read from the current
  // snapshot. Resets each time a new snapshot is taken.
  // Sould be in [0, adc_capture_snapshot.items.size()].
  uint16_t adc_capture_items_read_so_far = 0;
  analyzer::AdcCaptureBuffer adc_capture_snapshot;
  esp_gatt_rsp_t rsp = {};
};

static Vars vars;

struct ProtextedVars {
  // Set once, during initialization.
  uint16_t gatts_if = ESP_GATT_IF_NONE;

  // Set per connection.
  uint16_t conn_id = kInvalidConnId;
  bool state_notifications_enabled = false;
  // Track the optional connection WDT feature.
  // WDT is disabled if conn_wdt_period_millis is zero.
  uint32_t conn_wdt_period_millis = 0;
  uint32_t conn_wdt_timestamp_millis = 0;
};

// Protected by protected_vars_mutex.
static ProtextedVars protected_vars;

// static const uint16_t kPrimaryServiceDeclUuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint8_t kPrimaryServiceDeclUuid[] = {
    ENCODE_UUID_16(ESP_GATT_UUID_PRI_SERVICE)};

// static const uint16_t kCharDeclUuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t kCharDeclUuid[] = {
    ENCODE_UUID_16(ESP_GATT_UUID_CHAR_DECLARE)};

// static const uint16_t kChrConfigDeclUuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t kChrConfigDeclUuid[] = {
    ENCODE_UUID_16(ESP_GATT_UUID_CHAR_CLIENT_CONFIG)};

static const uint8_t kChrPropertyReadNotify =
    ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t kChrPropertyReadOnly = ESP_GATT_CHAR_PROP_BIT_READ;
// Caller can either use write request with or without response.
static const uint8_t kChrPropertyWriteOptionalResponse =
    ESP_GATT_CHAR_PROP_BIT_WRITE_NR | ESP_GATT_CHAR_PROP_BIT_WRITE;

// TODO: what does it do?
static uint8_t state_ccc_val[2] = {};

// TODO: why do we need this?
static uint8_t command_val[1] = {};

// Model string = "Stepper Probe ESP32"
static const uint8_t model_str_value[] = {'S', 't', 'e', 'p', 'p', 'e', 'r',
    ' ', 'P', 'r', 'o', 'b', 'e', ' ', 'E', 'S', 'P', '3', '2'};

// Revision string = "00.00.01"
static const uint8_t revision_str_value[] = {
    '0', '0', '.', '0', '0', '.', '0', '1'};

// Manufacturer string = "Zapta"
static const uint8_t manufacturer_str_value[] = {'Z', 'a', 'p', 't', 'a'};

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

  ATTR_IDX_COMMAND,
  ATTR_IDX_COMMAND_VAL,

  ATTR_IDX_CAPTURE,
  ATTR_IDX_CAPTURE_VAL,

  ATTR_IDX_COUNT,  // Attributes count.
};

// NOTE: The macros cast the data to non const as required by
// the API. Use const data with care.
#define LEN_BYTES(x) sizeof(x), (uint8_t*)(&(x))
#define LEN_LEN_BYTES(x) sizeof(x), sizeof(x), (uint8_t*)(&(x))

// The main BLE attribute table.
static const esp_gatts_attr_db_t attr_table[ATTR_IDX_COUNT] = {

    [ATTR_IDX_SVC] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kPrimaryServiceDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(service_uuid)}},

    // ----- Device Model.
    //
    // Characteristic
    [ATTR_IDX_MODEL] =

        {{ESP_GATT_AUTO_RSP},
            {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
                LEN_LEN_BYTES(kChrPropertyReadOnly)}},

    // Value.
    [ATTR_IDX_MODEL_VAL] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(model_uuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(model_str_value)}},

    // ----- Firmware revision
    //
    // Characteristic
    [ATTR_IDX_REVISION] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(kChrPropertyReadOnly)}},
    // Value.
    [ATTR_IDX_REVISION_VAL] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(revision_uuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(revision_str_value)}},

    // ----- Manufacturer name
    //
    // Characteristic
    [ATTR_IDX_MANUFECTURER] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(kChrPropertyReadOnly)}},
    // Value.
    [ATTR_IDX_MANUFECTURER_VAL] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(manufacturer_uuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(manufacturer_str_value)}},

    // ----- Probe info
    //
    // Characteristic
    [ATTR_IDX_PROBE_INFO] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(kChrPropertyReadOnly)}},

    // Value
    [ATTR_IDX_PROBE_INFO_VAL] = {{ESP_GATT_RSP_BY_APP},
        {LEN_BYTES(probe_info_uuid), ESP_GATT_PERM_READ, 0, 0, nullptr}},

    // ----- Stepper state.
    //
    // Characteristic
    [ATTR_IDX_STEPPER_STATE] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(kChrPropertyReadNotify)}},
    // Value
    [ATTR_IDX_STEPPER_STATE_VAL] = {{ESP_GATT_RSP_BY_APP},
        {LEN_BYTES(stepper_state_uuid), ESP_GATT_PERM_READ, 0, 0, nullptr}},

    // Client Characteristic Configuration Descriptor
    [ATTR_IDX_STEPPER_STATE_CCC] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kChrConfigDeclUuid),
            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            LEN_LEN_BYTES(state_ccc_val)}},

    // ----- Current histogram.
    //
    // Characteristic
    [ATTR_IDX_CURRENT_HISTOGRAM] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(kChrPropertyReadOnly)}},
    // Value
    [ATTR_IDX_CURRENT_HISTOGRAM_VAL] = {{ESP_GATT_RSP_BY_APP},
        {LEN_BYTES(current_histogram_uuid), ESP_GATT_PERM_READ, 0, 0, nullptr}},

    // ----- Time histogram.
    //
    // Characteristic
    [ATTR_IDX_TIME_HISTOGRAM] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(kChrPropertyReadOnly)}},
    // Value
    [ATTR_IDX_TIME_HISTOGRAM_VAL] = {{ESP_GATT_RSP_BY_APP},
        {LEN_BYTES(time_histogram_uuid), ESP_GATT_PERM_READ, 0, 0, nullptr}},

    // ----- Distance histogram.
    //
    // Characteristic
    [ATTR_IDX_DISTANCE_HISTOGRAM] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(kChrPropertyReadOnly)}},
    // Value
    [ATTR_IDX_DISTANCE_HISTOGRAM_VAL] = {{ESP_GATT_RSP_BY_APP},
        {LEN_BYTES(distance_histogram_uuid), ESP_GATT_PERM_READ, 0, 0,
            nullptr}},

    // ----- Command.
    //
    // Characteristic
    [ATTR_IDX_COMMAND] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(kChrPropertyWriteOptionalResponse)}},

    // Value
    [ATTR_IDX_COMMAND_VAL] = {{ESP_GATT_RSP_BY_APP},
        {LEN_BYTES(command_uuid), ESP_GATT_PERM_WRITE,
            LEN_LEN_BYTES(command_val)}},

    // ----- Capture.
    //
    // Characteristic
    [ATTR_IDX_CAPTURE] = {{ESP_GATT_AUTO_RSP},
        {LEN_BYTES(kCharDeclUuid), ESP_GATT_PERM_READ,
            LEN_LEN_BYTES(kChrPropertyReadOnly)}},

    // Value
    [ATTR_IDX_CAPTURE_VAL] = {{ESP_GATT_RSP_BY_APP},
        {LEN_BYTES(capture_uuid), ESP_GATT_PERM_READ, 0, 0, nullptr}},

};

// Parallel to the entries of attr_table.  Accessed only
// by the BLE thread callbacks and thus doesn't require
// a mutex protection.
uint16_t handle_table[ATTR_IDX_COUNT];

static esp_gatt_status_t on_probe_info_read(
    const gatts_read_evt_param& read_param, ble_util::Serializer* ser) {
  ESP_LOGI(TAG, "on_probe_info_read() called");

  assert(ser->size() == 0);
  ser->append_uint8(0x1);  // Packet format version
  ser->append_uint8(vars.hardware_config);
  ser->append_uint16(vars.adc_ticks_per_amp);
  ser->append_uint24(acq_consts::kTimeTicksPerSec);
  ser->append_uint16(acq_consts::kBucketStepsPerSecond);
  assert(ser->size() == 9);

  // Added March 2023. Not available in older versions.
  const char* app_info_str = util::app_version_str();
  ser->append_str(app_info_str);

  return ESP_GATT_OK;
}

// The max number of bytes in the response prefix.
static constexpr uint16_t kCaptureValuePrefixMaxLen = 9;

static esp_gatt_status_t on_capture_read(
    const gatts_read_evt_param& read_param, ble_util::Serializer* ser) {
  ESP_LOGD(TAG, "on_capture_read() called");

  assert(ser->size() == 0);
  const uint16_t max_bytes =
      std::min(vars.conn_mtu - kMtuOverhead, ser->capacity());
  ESP_LOGD(TAG, "on_capture_read(): max_len = %hu", max_bytes);

  // We reject even if currently we don't have enough pending
  // data to fill the max bytes.
  if (max_bytes < 100) {
    ESP_LOGE(TAG, "Capture read: max_len %hu is too small (mtu=%hu)", max_bytes,
        vars.conn_mtu);
    return ESP_GATT_OUT_OF_RANGE;
  }

  // Index of first item to transfer.
  const int start_item_index = vars.adc_capture_items_read_so_far;
  // How many left to transfer.
  const int desired_item_count =
      vars.adc_capture_snapshot.items.size() - start_item_index;
  // How many can we transfer now. Using 4 bytes per entry.
  const int available_item_count = (max_bytes - kCaptureValuePrefixMaxLen) / 4;
  // How many we are going to transfer now.
  const int actual_item_count = (desired_item_count <= available_item_count)
      ? desired_item_count
      : available_item_count;

  ESP_LOGD(TAG, "Capture read: start=%d, desired=%d, actual=%d",
      start_item_index, desired_item_count, actual_item_count);

  ser->append_uint8(0x40);  // format id.

  // Flags (uint8)
  uint8_t flags = 0x00;
  if (actual_item_count) {
    flags = flags | 0x80;  // Snapshot available.
    if (actual_item_count < desired_item_count) {
      flags = flags | 0x01;  // Needs at least one more read.
    }
  }

  ser->append_uint8(flags);

  if (actual_item_count) {
    ser->append_uint16(vars.adc_capture_snapshot.seq_number);
    ser->append_uint8(vars.adc_capture_snapshot.divider);
    ser->append_uint16((uint16_t)actual_item_count);
    ser->append_uint16((uint16_t)start_item_index);

    // Encode data points as pairs of int16_t.
    for (int i = start_item_index; i < start_item_index + actual_item_count;
         i++) {
      const analyzer::AdcCaptureItem* item =
          vars.adc_capture_snapshot.items.get(i);
      ser->append_int16(item->v1);
      ser->append_int16(item->v2);
    }

    // Update for next chunk read.
    vars.adc_capture_items_read_so_far += actual_item_count;
  }

  return ESP_GATT_OK;
}

static void serialize_state(
    const analyzer::State& state, ble_util::Serializer* ser) {
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
      (state.is_reverse_direction ? 0x10 : 0) | (state.quadrant & 0x03);

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
    const gatts_read_evt_param& read_param, ble_util::Serializer* ser) {
  ESP_LOGD(TAG, "on_stepper_state_read() called");

  analyzer::sample_state(&vars.stepper_state_buffer);
  serialize_state(vars.stepper_state_buffer, ser);

  return ESP_GATT_OK;
}

static esp_gatt_status_t on_current_histogram_read(
    const gatts_read_evt_param& read_param, ble_util::Serializer* ser) {
  ESP_LOGD(TAG, "on_current_histogram_read() called");

  analyzer::sample_histogram(&vars.histogram_buffer);

  assert(ser->size() == 0);
  ser->append_uint8(0x10);  // format id.

  ser->append_uint8(acq_consts::kNumHistogramBuckets);  // Num of buckets

  // Format bucket values, (2 bytes each)
  for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
    const analyzer::HistogramBucket& bucket = vars.histogram_buffer.buckets[i];
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
    const gatts_read_evt_param& read_param, ble_util::Serializer* ser) {
  ESP_LOGD(TAG, "on_time_histogram_read() called");

  analyzer::sample_histogram(&vars.histogram_buffer);

  assert(ser->size() == 0);
  ser->append_uint8(0x20);  // format id.
  ser->append_uint8(acq_consts::kNumHistogramBuckets);  // Num of buckets

  // Find total time value.
  uint64_t total_ticks = 0;
  for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
    total_ticks += vars.histogram_buffer.buckets[i].total_ticks_in_steps;
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
          (vars.histogram_buffer.buckets[i].total_ticks_in_steps * 1000) /
          total_ticks;
      ser->append_uint16(normalized_val);
    }
  }

  return ESP_GATT_OK;
}

static esp_gatt_status_t on_distance_histogram_read(
    const gatts_read_evt_param& read_param, ble_util::Serializer* ser) {
  ESP_LOGD(TAG, "on_distance_histogram_read() called");

  analyzer::sample_histogram(&vars.histogram_buffer);

  assert(ser->size() == 0);
  ser->append_uint8(0x30);  // Format id.
  ser->append_uint8(acq_consts::kNumHistogramBuckets);  // Num buckets

  // Find total steps.
  uint64_t total_steps = 0;
  for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
    total_steps += vars.histogram_buffer.buckets[i].total_steps;
  }

  if (total_steps < 10) {
    // Special case: When the total time is low, we just set the entire
    // histogram as zero. This also prevents divide by zero.
    for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
      ser->append_uint16(0);
    }
  } else {
    // Normal case: encide relative values as permils (0.1%) of the
    // total. [0, 1000].
    for (int i = 0; i < acq_consts::kNumHistogramBuckets; i++) {
      const uint16_t normalized_val =
          (vars.histogram_buffer.buckets[i].total_steps * 1000) / total_steps;
      ser->append_uint16(normalized_val);
    }
  }

  return ESP_GATT_OK;
}

static esp_gatt_status_t on_state_notification_control_write(
    const gatts_write_evt_param& write_param) {
  if (write_param.len != 2 || write_param.is_prep) {
    return ESP_GATT_ERROR;
  }

  const uint16_t descr_value = write_param.value[1] << 8 | write_param.value[0];
  const bool notifications_enabled = descr_value & 0x0001;

  ENTER_MUTEX {
    ESP_LOGI(TAG, "Notifications 0x%04x: %d -> %d", descr_value,
        protected_vars.state_notifications_enabled, notifications_enabled);
    protected_vars.state_notifications_enabled = notifications_enabled;
  }
  EXIT_MUTEX

  return ESP_GATT_OK;
}

// Ser is for encoding an optional response.
static esp_gatt_status_t on_command_write(
    const gatts_write_evt_param& write_param, ble_util::Serializer* ser) {
  ESP_LOGD(TAG, "on_command_write() called, values:");
  ESP_LOG_BUFFER_HEX_LEVEL(
      TAG, write_param.value, write_param.len, ESP_LOG_DEBUG);

  const uint8_t* data = write_param.value;
  const int len = write_param.len;

  // We need at least one byte for the opcode.
  if (len < 1) {
    ESP_LOGE(TAG, "on_command_write: empty command");
    return ESP_GATT_INVALID_ATTR_LEN;
  }

  const uint32_t opcode = data[0];

  switch (opcode) {
    // command = Reset data.
    case 0x01:
      if (len != 1) {
        ESP_LOGE(TAG, "Reset command too long: %hu", len);
        return ESP_GATT_INVALID_ATTR_LEN;
      }
      analyzer::reset_data();
      ESP_LOGI(TAG, "Stepper data reset.");
      return ESP_GATT_OK;

    // Command = Snapshot ADC signal capture.
    case 0x02:
      if (len != 1) {
        ESP_LOGE(TAG, "Signal capture command too long: %hu", len);
        return ESP_GATT_INVALID_ATTR_LEN;
      }
      analyzer::get_last_capture_snapshot(&vars.adc_capture_snapshot);
      vars.adc_capture_items_read_so_far = 0;
      ESP_LOGD(TAG, "ADC signal captured.");
      return ESP_GATT_OK;

    // Command = Set ADC capture divider. Note that until the new capture
    // will be ready, the last capture is still with the old divider.
    case 0x03:
      if (len != 2) {
        ESP_LOGE(TAG, "Set divider command wrong length : %hu", len);
        return ESP_GATT_INVALID_ATTR_LEN;
      }
      analyzer::set_signal_capture_divider(data[1]);
      ESP_LOGI(TAG, "signal capture divider set to %hhu", data[1]);

      return ESP_GATT_OK;

      // Command = toggle direction. This doesn't reverses the motors
      // buy just the direction of the step counting. New value is
      // persisted on the eeprom.
    case 0x04: {
      if (len != 1) {
        ESP_LOGE(TAG, "Toggle direction command wrong length : %hu", len);
        return ESP_GATT_INVALID_ATTR_LEN;
      }
      bool new_direction = false;
      if (!controls::toggle_direction(&new_direction)) {
        ESP_LOGE(TAG, "Direction change failed");
        return ESP_GATT_WRITE_NOT_PERMIT;
      }
      ESP_LOGI(TAG, "Drection toggled to %d", new_direction);
      return ESP_GATT_OK;
    }

      // Command = zero calibrate the sensors. Should we called with
      // zero sensor curent, preferably disconnected. New value
      // is persisted on the eeprom.
    case 0x05:
      if (len != 1) {
        ESP_LOGE(TAG, "zero calibration command wrong length : %hu", len);
        return ESP_GATT_INVALID_ATTR_LEN;
      }
      if (!controls::zero_calibration()) {
        ESP_LOGE(TAG, "Zero calibration failed");
        return ESP_GATT_WRITE_NOT_PERMIT;
      }
      ESP_LOGI(TAG, "Sensors zero calibrated.");
      return ESP_GATT_OK;

    // Command = wdt timer.
    case 0x06: {
      if (len != 2) {
        ESP_LOGE(TAG, "WDT command wrong length : %hu", len);
        return ESP_GATT_INVALID_ATTR_LEN;
      }
      ENTER_MUTEX {
        protected_vars.conn_wdt_period_millis = (uint32_t)data[1] * 1000;
        protected_vars.conn_wdt_timestamp_millis = util::time_ms();
      }
      EXIT_MUTEX

      // ESP_LOGI(TAG, "Conn WDT reset (%hhu secs)", data[1]);
      return ESP_GATT_OK;
    }

      // Command = set nickname.
    case 0x07: {
      ESP_LOGI(TAG, "Cmd 0x07");

      if (len < 2) {
        ESP_LOGE(TAG, "Set nickname command wrong length : %hu", len);
        return ESP_GATT_INVALID_ATTR_LEN;
      }

      const uint8_t str_len = data[1];

      // For now settings has a single field so we don't need to read the
      // other fields from nvs.
      nvs_config::BleSettings settings = {0};
      if (str_len >= sizeof(settings.nickname) || 2 + str_len > len) {
        ESP_LOGE(TAG, "Set nickname command wrong string length : %hhu (%d)",
            str_len, len);
        return ESP_GATT_INVALID_ATTR_LEN;
      }

      memcpy(settings.nickname, &data[2], str_len);
      settings.nickname[str_len] = '\0';
      ESP_LOGI(TAG, "Setting nickname: [%s]", settings.nickname);
      if (!nvs_config::write_ble_settings(settings)) {
        ESP_LOGE(TAG, "Set nickname command failed to write settings");
        return ESP_GATT_ERROR;
      }

      // Indicate that we need to re-register the updated advertisement
      // data, before we can start advertising it.
      vars.adv_data_configured = false;
      vars.scan_rsp_configured = false;

      // ESP_LOGI(TAG, "Conn WDT reset (%hhu secs)", data[1]);
      return ESP_GATT_OK;
    }

    default:
      ESP_LOGE(TAG, "on_command_write: unknown opcode: %02lx", opcode);
      return ESP_GATT_REQ_NOT_SUPPORTED;
  }
}

static void gap_event_handler(
    esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
  switch (event) {
    // Called once during setup to indicate that adv data is configured.
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
      ESP_LOGI(TAG, "Adv data configured");
      vars.adv_data_configured = true;
      if (vars.scan_rsp_configured) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;

    // Called once during setup to indicate that adv scan rsp is configured.
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
      ESP_LOGI(TAG, "Scan rsp configured");
      vars.scan_rsp_configured = true;
      if (vars.scan_rsp_configured) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;

      // #endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      // advertising start complete event to indicate advertising start
      // successfully or failed
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
        ESP_LOGI(TAG, "Stop adv successfully");
      }
      break;

    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
      ESP_LOGI(TAG,
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

static void update_adv_device_nickname() {
  nvs_config::BleSettings ble_settings;
  if (!nvs_config::read_ble_settings(&ble_settings)) {
    ble_settings = nvs_config::kDefaultBleDefaultSetting;
    ESP_LOGI(TAG, "No ble settings, using defaults");
  }
  ESP_LOGI(TAG, "Device nickname: [%s]", ble_settings.nickname);

  // Set vars.nickname. Truncate if too long.
  vars.manufacturer_data_len = 0;
  // We use an arbitrary BLE data type code. 4369 as decimal.
  vars.manufacturer_data[vars.manufacturer_data_len++] = 0x11;
  vars.manufacturer_data[vars.manufacturer_data_len++] = 0x11;

  // Copy with possible but unlikely truncation.
  const char* p;
  for (p = ble_settings.nickname;
       *p && vars.manufacturer_data_len < sizeof(vars.manufacturer_data); p++) {
    vars.manufacturer_data[vars.manufacturer_data_len++] = *p;
  }
  if (*p) {
    ESP_LOGE(TAG, "Nickname was too long, truncated.");
  }
  // Attached to scan data
  scan_rsp_data.p_manufacturer_data = &vars.manufacturer_data[0];
  scan_rsp_data.manufacturer_len = vars.manufacturer_data_len;
}

// Called in setup or whenever we need to start advertising after the connection
// changed the nickname.
static void register_adv() {
  update_adv_device_nickname();

  // Clear the flags that indicates the completion of the registrations.
  vars.adv_data_configured = false;
  vars.scan_rsp_configured = false;
  esp_err_t err = esp_ble_gap_config_adv_data(&adv_data);
  if (err) {
    ESP_LOGE(TAG, "config adv data failed, error code = %x", err);
  }
  err = esp_ble_gap_config_adv_data(&scan_rsp_data);
  if (err) {
    ESP_LOGE(TAG, "config scan response data failed, error code = %x", err);
  }
}

static void gatts_event_handler(esp_gatts_cb_event_t event,
    esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
  switch (event) {
    case ESP_GATTS_REG_EVT: {
      ESP_LOGI(TAG, "ESP_GATTS_REG_EVT event");
      assert(param->reg.status == ESP_GATT_OK);

      ENTER_MUTEX { protected_vars.gatts_if = gatts_if; }
      EXIT_MUTEX

      // Construct device name from device address.
      const uint8_t* device_addr = esp_bt_dev_get_address();
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

      register_adv();

      err = esp_ble_gatts_create_attr_tab(
          attr_table, gatts_if, ATTR_IDX_COUNT, SVC_INST_ID);
      if (err) {
        ESP_LOGE(TAG, "create attr table failed, error code = %x", err);
      }
    } break;

    // Handle read event.
    case ESP_GATTS_READ_EVT: {
      ESP_LOGD(TAG, "ESP_GATTS_READ_EVT");
      const gatts_read_evt_param& read_param = param->read;

      // For characteristics with auto resp we don't need to do
      // anything here.
      if (!read_param.need_rsp) {
        ESP_LOGD(TAG, "ESP_GATTS_READ_EVT, rsp not needed.");
        return;
      }

      // NOTE: rsp is large, such that this memset takes about 10us.
      // io::TEST2.set();
      memset(&vars.rsp, 0, sizeof(vars.rsp));
      // io::TEST2.clr();

      ble_util::Serializer ser(
          vars.rsp.attr_value.value, sizeof(vars.rsp.attr_value.value));
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
      } else if (read_param.handle == handle_table[ATTR_IDX_CAPTURE_VAL]) {
        status = on_capture_read(read_param, &ser);
      }

      const uint16_t len = (status == ESP_GATT_OK) ? ser.size() : 0;
      ESP_LOGD(TAG, "ESP_GATTS_READ_EVT: response status: %hu %s, len: %hu",
          status, ble_util::gatts_status_name(status), len);
      vars.rsp.attr_value.len = len;
      vars.rsp.attr_value.handle = read_param.handle;
      esp_ble_gatts_send_response(
          gatts_if, read_param.conn_id, read_param.trans_id, status, &vars.rsp);

    } break;

    // Write event
    case ESP_GATTS_WRITE_EVT: {
      const gatts_write_evt_param& write_param = param->write;
      ESP_LOGD(TAG,
          "ESP_GATTS_WRITE_EVT event: handle=%d, is_prep=%d, need_rsp=%d, "
          "len=%d, value:",
          write_param.handle, write_param.is_prep, write_param.need_rsp,
          write_param.len);
      ESP_LOG_BUFFER_HEX_LEVEL(
          TAG, write_param.value, write_param.len, ESP_LOG_DEBUG);

      // Handle by case.
      vars.rsp.attr_value.handle = write_param.handle;
      vars.rsp.attr_value.len = 0;
      esp_gatt_status_t status = ESP_GATT_OK;
      if (write_param.is_prep) {
        ESP_LOGW(TAG, "Unexpected is_prep write");
        status = ESP_GATT_REQ_NOT_SUPPORTED;
      } else if (write_param.offset != 0) {
        status = ESP_GATT_INVALID_OFFSET;
      } else if (handle_table[ATTR_IDX_STEPPER_STATE_CCC] ==
          write_param.handle) {
        status = on_state_notification_control_write(write_param);
      } else if (handle_table[ATTR_IDX_COMMAND_VAL] == write_param.handle) {
        ESP_LOGD(TAG,
            "Command write:  is_prep=%d, need_rsp=%d, "
            "len=%d, value:",
            write_param.is_prep, write_param.need_rsp, write_param.len);
        ble_util::Serializer ser(
            vars.rsp.attr_value.value, sizeof(vars.rsp.attr_value.value));
        status = on_command_write(write_param, &ser);
        vars.rsp.attr_value.len = (status == ESP_GATT_OK) ? ser.size() : 0;
      }

      // A write needs response if the characteristics is declared with non
      // auto response and the caller requested a response.
      if (write_param.need_rsp) {
        ESP_LOGI(TAG,
            "ESP_GATTS_WRITE_EVT: sending response status: %hu %s, len: %hu",
            status, ble_util::gatts_status_name(status),
            vars.rsp.attr_value.len);
        esp_ble_gatts_send_response(gatts_if, write_param.conn_id,
            write_param.trans_id, status, &vars.rsp);
      } else {
        ESP_LOGD(TAG, "Response not requested or auto.");
      }
    } break;

    case ESP_GATTS_MTU_EVT:
      ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, mtu set to %d", param->mtu.mtu);
      vars.conn_mtu = param->mtu.mtu;
      break;

    case ESP_GATTS_START_EVT:
      ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d",
          param->start.status, param->start.service_handle);
      break;

    case ESP_GATTS_CONNECT_EVT: {
      ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d, remove address:",
          param->connect.conn_id);
      esp_log_buffer_hex(
          TAG, param->connect.remote_bda, sizeof(param->connect.remote_bda));
      // Verify our assumption that kInvalidConnId can't be a valid id.
      assert(param->connect.conn_id != kInvalidConnId);

      ENTER_MUTEX {
        protected_vars.conn_id = param->connect.conn_id;
        protected_vars.state_notifications_enabled = false;
        protected_vars.conn_wdt_period_millis = 0;
        protected_vars.conn_wdt_timestamp_millis = 0;
      }
      EXIT_MUTEX

      vars.conn_mtu = 23;  // Initial BLE MTU.
      esp_ble_conn_update_params_t conn_params = {};
      memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
      // For the iOS system, please refer to Apple official documents about
      // the BLE connection parameters restrictions.
      conn_params.latency = 0;
      conn_params.max_int = 0x20;  // max_int = 0x20*1.25ms = 40ms
      conn_params.min_int = 0x10;  // min_int = 0x10*1.25ms = 20ms
      conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
      // start sent the update connection parameters to the peer device.
      esp_ble_gap_update_conn_params(&conn_params);
    } break;

    // On connection disconnection.
    case ESP_GATTS_DISCONNECT_EVT:
      ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x",
          param->disconnect.reason);
      vars.conn_mtu = 0;

      ENTER_MUTEX {
        protected_vars.conn_id = kInvalidConnId;
        protected_vars.state_notifications_enabled = false;
        protected_vars.conn_wdt_period_millis = 0;
        protected_vars.conn_wdt_timestamp_millis = 0;
      }
      EXIT_MUTEX

      // Start advertising.
      if (vars.adv_data_configured && vars.scan_rsp_configured) {
        // Here advertisement data didn't change during the connection.
        esp_ble_gap_start_advertising(&adv_params);
      } else {
        // Adv data changed. Register the latest values.
        vars.adv_data_configured = false;
        vars.scan_rsp_configured = false;
        register_adv();
      }
      break;

    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
      if (param->add_attr_tab.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "create attribute table failed, error code=0x%x",
            param->add_attr_tab.status);
      } else if (param->add_attr_tab.num_handle != ATTR_IDX_COUNT) {
        ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)",
            param->add_attr_tab.num_handle, ATTR_IDX_COUNT);
      } else {
        ESP_LOGI(TAG, "create attribute table successfully, num handles = %d",
            param->add_attr_tab.num_handle);
        memcpy(handle_table, param->add_attr_tab.handles, sizeof(handle_table));
        esp_ble_gatts_start_service(handle_table[ATTR_IDX_SVC]);
      }
      break;
    }

    // TODO: explain here when this is called. Is it just for
    // app provided responses?
    case ESP_GATTS_RESPONSE_EVT:
      if (param->rsp.status == ESP_GATT_OK)
        ESP_LOGD(TAG, "ESP_GATTS_RESPONSE_EVT rsp OK");
      else {
        ESP_LOGW(
            TAG, "ESP_GATTS_RESPONSE_EVT rsp error: %d", param->rsp.status);
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

bool is_connected() {

  ProtextedVars prot_vars;
  ENTER_MUTEX { prot_vars = protected_vars; }
  EXIT_MUTEX

  const bool connected = prot_vars.conn_id != kInvalidConnId;
  const bool wdt_active = connected && (prot_vars.conn_wdt_period_millis != 0);
  const uint32_t wdt_elapsed_millis =
      wdt_active ? (util::time_ms() - prot_vars.conn_wdt_timestamp_millis) : 0;
  const bool wdt_expired =
      wdt_active && (wdt_elapsed_millis > prot_vars.conn_wdt_period_millis);

  // printf("%u, %u\n", wdt_elapsed_millis);
  if (wdt_expired) {
    // Disable
    // ENTER_MUTEX { protected_vars.conn_wdt_period_millis = 0; }
    EXIT_MUTEX
    const esp_err_t ret =
        esp_ble_gatts_close(prot_vars.gatts_if, prot_vars.conn_id);

    ESP_LOGW(TAG, "Disconnecting due to conn WDT (%lu ms). Status: %s",
        wdt_elapsed_millis, esp_err_to_name(ret));
  }

  // Since we didn't get yet a confirmation for the disconnection, we still
  // report connected.
  return connected;
}

void setup(uint8_t hardware_config, uint16_t adc_ticks_per_amp) {
  protected_vars_mutex = xSemaphoreCreateMutex();
  assert(protected_vars_mutex);

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  vars.hardware_config = hardware_config;
  vars.adc_ticks_per_amp = adc_ticks_per_amp;

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_err_t ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(
        TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
    assert(0);
  }

  // TODO: Understand why esp_bt_controller_enable sometimes crashes on reboot.
  // Observed on IDF 5.0.
  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(
        TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
    assert(0);
  }

  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(
        TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
    assert(0);
  }

  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(
        TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
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
  ret = esp_ble_gatt_set_local_mtu(kMaxRequestedMtu);
  if (ret) {
    ESP_LOGE(TAG, "set local  MTU failed, error code = %x", ret);
    assert(0);
  }
}

static uint8_t state_notification_buffer[50] = {};

void notify_state_if_enabled(const analyzer::State& state) {
  // Snapshot protected vars in a mutec.
  ProtextedVars prot_vars;
  ENTER_MUTEX { prot_vars = protected_vars; }
  EXIT_MUTEX

  // const bool notification_enabled = state_ccc_val[0] & 0x01;
  if (!prot_vars.state_notifications_enabled) {
    return;
  }

  assert(prot_vars.gatts_if != ESP_GATT_IF_NONE);
  assert(prot_vars.conn_id != kInvalidConnId);

  ble_util::Serializer ser(
      state_notification_buffer, sizeof(state_notification_buffer));
  serialize_state(state, &ser);

  // NOTE: need_config == false to indicate a notification (vs. indication).
  const esp_err_t err = esp_ble_gatts_send_indicate(prot_vars.gatts_if,
      prot_vars.conn_id, handle_table[ATTR_IDX_STEPPER_STATE_VAL], ser.size(),
      state_notification_buffer, false);

  if (err) {
    ESP_LOGE(TAG, "esp_ble_gatts_send_indicate() returned err 0x%x %s", err,
        esp_err_to_name(err));
  }
}

}  // namespace ble_host