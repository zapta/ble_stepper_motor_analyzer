#include "enum_code_gen.h"

// #include <map>
// #include "string.h"

namespace enum_code_gen {

// static constexpr auto TAG = "ble_util";

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])

#define ENTRY(x) \
  { x, #x }

struct ListEntry {
  // For verification. Should match the entry's index.
  int value;
  const char* name;
};

// Names of esp_gatts_cb_event_t
static const ListEntry gatts_events_list[] = {
    ENTRY(ESP_GATTS_REG_EVT),
    ENTRY(ESP_GATTS_READ_EVT),
    ENTRY(ESP_GATTS_WRITE_EVT),
    ENTRY(ESP_GATTS_EXEC_WRITE_EVT),
    ENTRY(ESP_GATTS_MTU_EVT),
    ENTRY(ESP_GATTS_CONF_EVT),
    ENTRY(ESP_GATTS_UNREG_EVT),
    ENTRY(ESP_GATTS_CREATE_EVT),
    ENTRY(ESP_GATTS_ADD_INCL_SRVC_EVT),
    ENTRY(ESP_GATTS_ADD_CHAR_EVT),
    ENTRY(ESP_GATTS_ADD_CHAR_DESCR_EVT),
    ENTRY(ESP_GATTS_DELETE_EVT),
    ENTRY(ESP_GATTS_START_EVT),
    ENTRY(ESP_GATTS_STOP_EVT),
    ENTRY(ESP_GATTS_CONNECT_EVT),
    ENTRY(ESP_GATTS_DISCONNECT_EVT),
    ENTRY(ESP_GATTS_OPEN_EVT),
    ENTRY(ESP_GATTS_CANCEL_OPEN_EVT),
    ENTRY(ESP_GATTS_CLOSE_EVT),
    ENTRY(ESP_GATTS_LISTEN_EVT),
    ENTRY(ESP_GATTS_CONGEST_EVT),
    ENTRY(ESP_GATTS_RESPONSE_EVT),
    ENTRY(ESP_GATTS_CREAT_ATTR_TAB_EVT),
    ENTRY(ESP_GATTS_SET_ATTR_VAL_EVT),
    ENTRY(ESP_GATTS_SEND_SERVICE_CHANGE_EVT),
};

// static const char* gatts_events_table[30] = {};

// Names of esp_gap_ble_cb_event_t.
static const ListEntry gap_ble_events_list[] = {
    ENTRY(ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SCAN_RESULT_EVT),
    ENTRY(ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_ADV_START_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_AUTH_CMPL_EVT),
    ENTRY(ESP_GAP_BLE_KEY_EVT),
    ENTRY(ESP_GAP_BLE_SEC_REQ_EVT),
    ENTRY(ESP_GAP_BLE_PASSKEY_NOTIF_EVT),
    ENTRY(ESP_GAP_BLE_PASSKEY_REQ_EVT),
    ENTRY(ESP_GAP_BLE_OOB_REQ_EVT),
    ENTRY(ESP_GAP_BLE_LOCAL_IR_EVT),
    ENTRY(ESP_GAP_BLE_LOCAL_ER_EVT),
    ENTRY(ESP_GAP_BLE_NC_REQ_EVT),
    ENTRY(ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT),
    ENTRY(ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT),
    ENTRY(ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SET_CHANNELS_EVT),
    ENTRY(ESP_GAP_BLE_READ_PHY_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SET_PREFERED_DEFAULT_PHY_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SET_PREFERED_PHY_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_ADV_SET_REMOVE_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_ADV_SET_CLEAR_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_DATA_SET_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_START_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_STOP_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_CREATE_SYNC_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_SYNC_CANCEL_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_SYNC_TERMINATE_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_ADD_DEV_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_REMOVE_DEV_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_CLEAR_DEV_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_SET_EXT_SCAN_PARAMS_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PREFER_EXT_CONN_PARAMS_SET_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_PHY_UPDATE_COMPLETE_EVT),
    ENTRY(ESP_GAP_BLE_EXT_ADV_REPORT_EVT),
    ENTRY(ESP_GAP_BLE_SCAN_TIMEOUT_EVT),
    ENTRY(ESP_GAP_BLE_ADV_TERMINATED_EVT),
    ENTRY(ESP_GAP_BLE_SCAN_REQ_RECEIVED_EVT),
    ENTRY(ESP_GAP_BLE_CHANNEL_SELETE_ALGORITHM_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_REPORT_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_SYNC_LOST_EVT),
    ENTRY(ESP_GAP_BLE_PERIODIC_ADV_SYNC_ESTAB_EVT),
    ENTRY(ESP_GAP_BLE_EVT_MAX),
};

// static const char* gap_ble_events_table[80] = {};

// Gatts status.
static const ListEntry gatts_status_list[] = {
    ENTRY(ESP_GATT_OK),
    ENTRY(ESP_GATT_INVALID_HANDLE),
    ENTRY(ESP_GATT_READ_NOT_PERMIT),
    ENTRY(ESP_GATT_WRITE_NOT_PERMIT),
    ENTRY(ESP_GATT_INVALID_PDU),
    ENTRY(ESP_GATT_INSUF_AUTHENTICATION),
    ENTRY(ESP_GATT_REQ_NOT_SUPPORTED),
    ENTRY(ESP_GATT_INVALID_OFFSET),
    ENTRY(ESP_GATT_INSUF_AUTHORIZATION),
    ENTRY(ESP_GATT_PREPARE_Q_FULL),
    ENTRY(ESP_GATT_NOT_FOUND),
    ENTRY(ESP_GATT_NOT_LONG),
    ENTRY(ESP_GATT_INSUF_KEY_SIZE),
    ENTRY(ESP_GATT_INVALID_ATTR_LEN),
    ENTRY(ESP_GATT_ERR_UNLIKELY),
    ENTRY(ESP_GATT_INSUF_ENCRYPTION),
    ENTRY(ESP_GATT_UNSUPPORT_GRP_TYPE),
    ENTRY(ESP_GATT_INSUF_RESOURCE),
    ENTRY(ESP_GATT_NO_RESOURCES),
    ENTRY(ESP_GATT_INTERNAL_ERROR),
    ENTRY(ESP_GATT_WRONG_STATE),
    ENTRY(ESP_GATT_DB_FULL),
    ENTRY(ESP_GATT_BUSY),
    ENTRY(ESP_GATT_ERROR),
    ENTRY(ESP_GATT_CMD_STARTED),
    ENTRY(ESP_GATT_ILLEGAL_PARAMETER),
    ENTRY(ESP_GATT_PENDING),
    ENTRY(ESP_GATT_AUTH_FAIL),
    ENTRY(ESP_GATT_MORE),
    ENTRY(ESP_GATT_INVALID_CFG),
    ENTRY(ESP_GATT_SERVICE_STARTED),
    ENTRY(ESP_GATT_ENCRYPED_MITM),
    ENTRY(ESP_GATT_ENCRYPED_NO_MITM),
    ENTRY(ESP_GATT_NOT_ENCRYPTED),
    ENTRY(ESP_GATT_CONGESTED),
    ENTRY(ESP_GATT_DUP_REG),
    ENTRY(ESP_GATT_ALREADY_OPEN),
    ENTRY(ESP_GATT_CANCEL),
    ENTRY(ESP_GATT_STACK_RSP),
    ENTRY(ESP_GATT_APP_RSP),
    ENTRY(ESP_GATT_UNKNOWN_ERROR),
    ENTRY(ESP_GATT_CCC_CFG_ERR),
    ENTRY(ESP_GATT_PRC_IN_PROGRESS),
    ENTRY(ESP_GATT_OUT_OF_RANGE)};

// static const char* gatts_status_table[256] = {};

// Common function to lookup a name in a table.
// static const char* lookup_name(int value, const char* table[], int
// table_size) {
//   if (value < 0 && value >= table_size) {
//     return "(invalid)";
//   }
//   // Sanity check.
//   return table[value];
// }

// const char* gatts_event_name(esp_gatts_cb_event_t event) {
//   return lookup_name(event, gatts_events_table,
//   ARRAY_SIZE(gatts_events_table));
// }

// const char* gap_ble_event_name(esp_gap_ble_cb_event_t event) {
//   return lookup_name(event, gap_ble_events_table,
//                      ARRAY_SIZE(gap_ble_events_table));
// }

// const char* gatts_status_name(esp_gatt_status_t status) {
//   return lookup_name(status, gatts_status_table,
//                      ARRAY_SIZE(gatts_status_table));
// }

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

static void gen_table_code(const char* table_name, int table_size,
                           const ListEntry list[], int list_size) {
  printf("\n\nstatic const char* %s[%d] = {\n", table_name, table_size);
  for (int i = 0; i < table_size; i++) {
    const char* name = nullptr;
    for (int j = 0; j < list_size; j++) {
      if (list[j].value == i) {
        assert(name == nullptr);  // Duplicate value
        name = list[j].name;
        break;
      }
    }
    printf("  \"%s\",\n", name ? name : "nullptr");
  }
  printf("};\n\n");
}

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

void gen_tables_code() {
  gen_table_code("gatts_events_table", 30, gatts_events_list,
                 ARRAY_SIZE(gatts_events_list));

  gen_table_code("gap_ble_events_table", 80, gap_ble_events_list,
                 ARRAY_SIZE(gap_ble_events_list));

  gen_table_code("gatts_status_table", 256, gatts_status_list,
                 ARRAY_SIZE(gatts_status_list));
}

}  // namespace enum_code_gen