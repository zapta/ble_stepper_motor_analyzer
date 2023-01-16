#include "ble/ble_service.h"

/*
  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_gatts.html

  https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/ble/gatt_server

  https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/bluedroid/ble/gatt_server_service_table/tutorial/Gatt_Server_Service_Table_Example_Walkthrough.md
*/

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_system.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_bt.h"
// // #include "bta_api.h"

// #include "esp_gap_ble_api.h"
// #include "esp_gatts_api.h"
// #include "esp_bt_defs.h"
// #include "esp_bt_main.h"
// #include "esp_bt_main.h"
// // #include "gatts_table_creat_demo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_defs.h"
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
#include "sdkconfig.h"

#define GATTS_TAG "GATTS_DEMO"

namespace ble_service {

// static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event,
// esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
//     // TBD
// }
// static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event,
// esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
//     // TBD
// }

// #define PROFILE_NUM 2
#define PROFILE_A_APP_ID 0

// #define GATTS_NUM_HANDLE_TEST_A 4

// #define PROFILE_NUM 2
// #define PROFILE_A_APP_ID 0
// #define PROFILE_B_APP_ID 1

#define GATTS_SERVICE_UUID_TEST_A   0x00FF
#define GATTS_CHAR_UUID_TEST_A      0xFF01
#define GATTS_DESCR_UUID_TEST_A     0x3333
#define GATTS_NUM_HANDLE_TEST_A     4

#define TEST_DEVICE_NAME "ESP_GATTS_DEMO"

static uint8_t adv_config_done = 0;
#define adv_config_flag (1 << 0)
#define scan_rsp_config_flag (1 << 1)

static esp_gatt_char_prop_t a_property = 0;

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

static uint8_t char1_str[] = {0x11,0x22,0x33};

static esp_attr_value_t gatts_demo_char1_val =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

static uint8_t adv_service_uuid128[32] = {
    /* LSB
       <-------------------------------------------------------------------------------->
       MSB */
    // first uuid, 16bit, [12],[13] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xEE,
    0x00,
    0x00,
    0x00,
    // second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xFF,
    0x00,
    0x00,
    0x00,
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
#pragma GCC diagnostic pop


struct gatts_profile_inst_t {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,  // slave connection min interval, Time =
                             // min_interval * 1.25 msec
    .max_interval = 0x0010,  // slave connection max interval, Time =
                             // max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,        // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL,  //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,        // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL,  //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
#pragma GCC diagnostic pop

// Used to be called gatts_profile_a_event_handler
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {
  printf("gatts_profile_a_event_handler called.");
}

/* One gatt-based profile one app_id and one gatts_if, this array will store the
 * gatts_if returned by ESP_GATTS_REG_EVT */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static gatts_profile_inst_t gl_profile = {
    .gatts_cb = gatts_profile_event_handler,
    .gatts_if = ESP_GATT_IF_NONE, 
};
#pragma GCC diagnostic pop



static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param) {
  printf("GAP handler called.\n");

  switch (event) {
#ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
      adv_config_done &= (~adv_config_flag);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
      adv_config_done &= (~scan_rsp_config_flag);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
#else
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
      adv_config_done &= (~adv_config_flag);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
      adv_config_done &= (~scan_rsp_config_flag);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
#endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      // advertising start complete event to indicate advertising start
      // successfully or failed
      if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
      }
      break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
      if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
      } else {
        ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
      }
      break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
      ESP_LOGI(
          GATTS_TAG,
          "update connection params status = %d, min_int = %d, max_int = "
          "%d,conn_int = %d,latency = %d, timeout = %d",
          param->update_conn_params.status, param->update_conn_params.min_int,
          param->update_conn_params.max_int, param->update_conn_params.conn_int,
          param->update_conn_params.latency, param->update_conn_params.timeout);
      break;
    default:
      printf("gap_event_handler: unknown event: %d\n", event);
      break;
  }
}

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param) {
  printf("GATT handle called.\n");

  switch (event) {
    case ESP_GATTS_REG_EVT: {
      ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n",
               param->reg.status, param->reg.app_id);
      gl_profile.service_id.is_primary = true;
      gl_profile.service_id.id.inst_id = 0x00;
      gl_profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
      gl_profile.service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

      esp_err_t set_dev_name_ret =
          esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
      if (set_dev_name_ret) {
        ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x",
                 set_dev_name_ret);
      }
#ifdef CONFIG_SET_RAW_ADV_DATA
      esp_err_t raw_adv_ret =
          esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
      if (raw_adv_ret) {
        ESP_LOGE(GATTS_TAG, "config raw adv data failed, error code = %x ",
                 raw_adv_ret);
      }
      adv_config_done |= adv_config_flag;
      esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(
          raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
      if (raw_scan_ret) {
        ESP_LOGE(GATTS_TAG, "config raw scan rsp data failed, error code = %x",
                 raw_scan_ret);
      }
      adv_config_done |= scan_rsp_config_flag;
#else
      // config adv data
      esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
      if (ret) {
        ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
      }
      adv_config_done |= adv_config_flag;
      // config scan response data
      ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
      if (ret) {
        ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x",
                 ret);
      }
      adv_config_done |= scan_rsp_config_flag;

#endif
      esp_ble_gatts_create_service(gatts_if, &gl_profile.service_id,
                                   GATTS_NUM_HANDLE_TEST_A);
    } break;

    case ESP_GATTS_CREATE_EVT: {
      ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n",
               param->create.status, param->create.service_handle);
      gl_profile.service_handle = param->create.service_handle;
      gl_profile.char_uuid.len = ESP_UUID_LEN_16;
      gl_profile.char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

      esp_ble_gatts_start_service(gl_profile.service_handle);
      a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE |
                   ESP_GATT_CHAR_PROP_BIT_NOTIFY;
      esp_err_t add_char_ret = esp_ble_gatts_add_char(
          gl_profile.service_handle, &gl_profile.char_uuid,
          ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, a_property,
          &gatts_demo_char1_val, NULL);
      if (add_char_ret) {
        ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_char_ret);
      }
    } break;

    default:
      printf("Unknown gatt handler event: %d\n", event);
      break;
  }
}

// Requires nvs to be initialized.
void setup() {
  esp_err_t ret;


  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__,
             esp_err_to_name(ret));
    return;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__,
             esp_err_to_name(ret));
    return;
  }

  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__,
             esp_err_to_name(ret));
    return;
  }
  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__,
             esp_err_to_name(ret));
    return;
  }

  ret = esp_ble_gatts_register_callback(gatts_event_handler);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
    return;
  }

  ret = esp_ble_gap_register_callback(gap_event_handler);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
    return;
  }

  ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
    return;
  }

  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
  if (local_mtu_ret != ESP_OK) {
    ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x",
             local_mtu_ret);
  }

  return;
}

}  // namespace ble_service