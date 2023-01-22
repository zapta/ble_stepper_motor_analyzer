#include "ble_util.h"

// #include <map>
// #include "string.h"

namespace ble_util {

static constexpr auto TAG = "ble_util";

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])

// These tables where generated automatically using the
// enum_code_gen library. Do not edit manually.

static const char* gatts_events_table[30] = {
    "ESP_GATTS_REG_EVT",
    "ESP_GATTS_READ_EVT",
    "ESP_GATTS_WRITE_EVT",
    "ESP_GATTS_EXEC_WRITE_EVT",
    "ESP_GATTS_MTU_EVT",
    "ESP_GATTS_CONF_EVT",
    "ESP_GATTS_UNREG_EVT",
    "ESP_GATTS_CREATE_EVT",
    "ESP_GATTS_ADD_INCL_SRVC_EVT",
    "ESP_GATTS_ADD_CHAR_EVT",
    "ESP_GATTS_ADD_CHAR_DESCR_EVT",
    "ESP_GATTS_DELETE_EVT",
    "ESP_GATTS_START_EVT",
    "ESP_GATTS_STOP_EVT",
    "ESP_GATTS_CONNECT_EVT",
    "ESP_GATTS_DISCONNECT_EVT",
    "ESP_GATTS_OPEN_EVT",
    "ESP_GATTS_CANCEL_OPEN_EVT",
    "ESP_GATTS_CLOSE_EVT",
    "ESP_GATTS_LISTEN_EVT",
    "ESP_GATTS_CONGEST_EVT",
    "ESP_GATTS_RESPONSE_EVT",
    "ESP_GATTS_CREAT_ATTR_TAB_EVT",
    "ESP_GATTS_SET_ATTR_VAL_EVT",
    "ESP_GATTS_SEND_SERVICE_CHANGE_EVT",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
};

static const char* gap_ble_events_table[80] = {
    "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT",
    "ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT",
    "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT",
    "ESP_GAP_BLE_SCAN_RESULT_EVT",
    "ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT",
    "ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT",
    "ESP_GAP_BLE_ADV_START_COMPLETE_EVT",
    "ESP_GAP_BLE_SCAN_START_COMPLETE_EVT",
    "ESP_GAP_BLE_AUTH_CMPL_EVT",
    "ESP_GAP_BLE_KEY_EVT",
    "ESP_GAP_BLE_SEC_REQ_EVT",
    "ESP_GAP_BLE_PASSKEY_NOTIF_EVT",
    "ESP_GAP_BLE_PASSKEY_REQ_EVT",
    "ESP_GAP_BLE_OOB_REQ_EVT",
    "ESP_GAP_BLE_LOCAL_IR_EVT",
    "ESP_GAP_BLE_LOCAL_ER_EVT",
    "ESP_GAP_BLE_NC_REQ_EVT",
    "ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT",
    "ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT",
    "ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT",
    "ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT",
    "ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT",
    "ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT",
    "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT",
    "ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT",
    "ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT",
    "ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT",
    "ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT",
    "ESP_GAP_BLE_UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE_EVT",
    "ESP_GAP_BLE_SET_CHANNELS_EVT",
    "ESP_GAP_BLE_READ_PHY_COMPLETE_EVT",
    "ESP_GAP_BLE_SET_PREFERED_DEFAULT_PHY_COMPLETE_EVT",
    "ESP_GAP_BLE_SET_PREFERED_PHY_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_ADV_SET_REMOVE_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_ADV_SET_CLEAR_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_DATA_SET_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_START_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_STOP_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_CREATE_SYNC_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_SYNC_CANCEL_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_ADD_DEV_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_REMOVE_DEV_COMPLETE_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_CLEAR_DEV_COMPLETE_EVT",
    "ESP_GAP_BLE_SET_EXT_SCAN_PARAMS_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT",
    "ESP_GAP_BLE_PREFER_EXT_CONN_PARAMS_SET_COMPLETE_EVT",
    "ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT",
    "ESP_GAP_BLE_EXT_ADV_REPORT_EVT",
    "ESP_GAP_BLE_SCAN_TIMEOUT_EVT",
    "ESP_GAP_BLE_ADV_TERMINATED_EVT",
    "ESP_GAP_BLE_SCAN_REQ_RECEIVED_EVT",
    "ESP_GAP_BLE_CHANNEL_SELETE_ALGORITHM_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_REPORT_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_SYNC_LOST_EVT",
    "ESP_GAP_BLE_PERIODIC_ADV_SYNC_ESTAB_EVT",
    "ESP_GAP_BLE_EVT_MAX",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
};

static const char* gatts_status_table[256] = {
    "ESP_GATT_OK",
    "ESP_GATT_INVALID_HANDLE",
    "ESP_GATT_READ_NOT_PERMIT",
    "ESP_GATT_WRITE_NOT_PERMIT",
    "ESP_GATT_INVALID_PDU",
    "ESP_GATT_INSUF_AUTHENTICATION",
    "ESP_GATT_REQ_NOT_SUPPORTED",
    "ESP_GATT_INVALID_OFFSET",
    "ESP_GATT_INSUF_AUTHORIZATION",
    "ESP_GATT_PREPARE_Q_FULL",
    "ESP_GATT_NOT_FOUND",
    "ESP_GATT_NOT_LONG",
    "ESP_GATT_INSUF_KEY_SIZE",
    "ESP_GATT_INVALID_ATTR_LEN",
    "ESP_GATT_ERR_UNLIKELY",
    "ESP_GATT_INSUF_ENCRYPTION",
    "ESP_GATT_UNSUPPORT_GRP_TYPE",
    "ESP_GATT_INSUF_RESOURCE",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "ESP_GATT_NO_RESOURCES",
    "ESP_GATT_INTERNAL_ERROR",
    "ESP_GATT_WRONG_STATE",
    "ESP_GATT_DB_FULL",
    "ESP_GATT_BUSY",
    "ESP_GATT_ERROR",
    "ESP_GATT_CMD_STARTED",
    "ESP_GATT_ILLEGAL_PARAMETER",
    "ESP_GATT_PENDING",
    "ESP_GATT_AUTH_FAIL",
    "ESP_GATT_MORE",
    "ESP_GATT_INVALID_CFG",
    "ESP_GATT_SERVICE_STARTED",
    "ESP_GATT_ENCRYPED_NO_MITM",
    "ESP_GATT_NOT_ENCRYPTED",
    "ESP_GATT_CONGESTED",
    "ESP_GATT_DUP_REG",
    "ESP_GATT_ALREADY_OPEN",
    "ESP_GATT_CANCEL",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "ESP_GATT_STACK_RSP",
    "ESP_GATT_APP_RSP",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "ESP_GATT_UNKNOWN_ERROR",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "nullptr",
    "ESP_GATT_CCC_CFG_ERR",
    "ESP_GATT_PRC_IN_PROGRESS",
    "ESP_GATT_OUT_OF_RANGE",
};

// Common function to lookup a name in a table.
static inline const char* lookup_name(int value, const char* table[], int table_size) {
  if (value < 0 && value >= table_size) {
    return "(invalid)";
  }
  // Sanity check.
  return table[value];
}

const char* gatts_event_name(esp_gatts_cb_event_t event) {
  return lookup_name(event, gatts_events_table, ARRAY_SIZE(gatts_events_table));
}

const char* gap_ble_event_name(esp_gap_ble_cb_event_t event) {
  return lookup_name(event, gap_ble_events_table,
                     ARRAY_SIZE(gap_ble_events_table));
}

const char* gatts_status_name(esp_gatt_status_t status) {
  return lookup_name(status, gatts_status_table,
                     ARRAY_SIZE(gatts_status_table));
}

// static void test(const ListEntry* table, int table_size) {
//   for (int i = 0; i < table_size; i++) {
//     assert(table[i].value == i);
//   }
// }
// void test_tables() {
//   test(gatts_events_list, ARRAY_SIZE(gatts_events_list));
//   test(gap_ble_events_list, ARRAY_SIZE(gap_ble_events_list));
// }

// This list is sparse in the sense that the numeric values don't match the
// array indexes. We use it during setup to build a proper table.

// std::map<int, const char*> gatt_status_map;

// static const char* gatts_status_table[256] = {};

// Populate a table from a list. In a table, the value of the entries match
// their index to speedup lookup.
// static void populate_table(const char* description, const ListEntry list[],
//                            const char* table[], int list_size, int
//                            table_size) {
//   // Assuming that all entries in the table are cleared to nullptr;
//   for (int i = 0; i < list_size; i++) {
//     const ListEntry& entry = list[i];
//     if (entry.value < 0 || entry.value >= table_size) {
//       // This will result in this value looked up as '(unknown)'.
//       // Not a big deal but increase table size if reasonable.
//       ESP_LOGW(TAG, "Dropping %s %s", description, entry.name);
//     } else {
//       table[i] = entry.name;
//     }
//   }
// }

// static void gen_table_code(const char* table_name, int table_size,
//                            const ListEntry list[], int list_size) {
//   printf("\nstatic const char* %s[%d] = {\n", table_name, table_size);
//   for (int i = 0; i < table_size; i++) {
//     const char* name = nullptr;
//     for (int j = 0; j < list_size; j++) {
//       if (list[j].value == i) {
//         name = list[j].name;
//         break;
//       }
//       printf("  \"%s\",\n", name ? name : "nullptr");
//     }
//   }
//   printf("};\n\n");
// }

// const char* gatts_status_name(esp_gatt_status_t status) {
//   if (status < 0 || status > 256 || !gatts_status_table[status]) {
//     return "(unknown)";
//   }
//   return gatts_status_table[status];
// }

// void setup() {
//   populate_table("gatts event", gatts_events_list, gatts_events_table,
//                  ARRAY_SIZE(gatts_events_list),
//                  ARRAY_SIZE(gatts_events_table));
//   populate_table("gap_ble_event", gap_ble_events_list, gap_ble_events_table,
//                  ARRAY_SIZE(gap_ble_events_list),
//                  ARRAY_SIZE(gap_ble_events_table));
//   populate_table("gatts_status", gatts_status_list, gatts_status_table,
//                  ARRAY_SIZE(gatts_status_list),
//                  ARRAY_SIZE(gatts_status_table));
// }

// void gen_tables_code() {
//   gen_table_code("gatts event_table", 30, gatts_events_list,
//                  ARRAY_SIZE(gatts_events_list));
// }

}  // namespace ble_util