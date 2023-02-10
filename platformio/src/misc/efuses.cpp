#include "util.h"

#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_log.h"

namespace efuses {

static constexpr auto TAG = "efuses";

struct EfuseItem {
  const esp_efuse_desc_t** desc;
  const char* name;
};

// Efuses fields to dump.
static EfuseItem efuse_items[] = {
    {.desc = ESP_EFUSE_CHIP_VER_REV1, .name = "CHIP_VER_REV1"},
    {.desc = ESP_EFUSE_CHIP_VER_REV2, .name = "CHIP_VER_REV2"},
    {.desc = ESP_EFUSE_CHIP_VER_DIS_BT, .name = "CHIP_VER_DIS_BT"},
    {.desc = ESP_EFUSE_CHIP_VER_PKG, .name = "CHIP_VER_PKG"},
    {.desc = ESP_EFUSE_DISABLE_JTAG, .name = "DISABLE_JTAG"},
    {.desc = ESP_EFUSE_CONSOLE_DEBUG_DISABLE, .name = "CONSOLE_DEBUG_DISABLE"},
    {.desc = ESP_EFUSE_CHIP_VER_DIS_APP_CPU, .name = "CHIP_VER_DIS_APP_CPU"},
    {.desc = ESP_EFUSE_CHIP_VER_DIS_BT, .name = "CHIP_VER_DIS_BT"},
    {.desc = ESP_EFUSE_CONSOLE_DEBUG_DISABLE, .name = "CONSOLE_DEBUG_DISABLE"},
    {.desc = ESP_EFUSE_UART_DOWNLOAD_DIS, .name = "UART_DOWNLOAD_DIS"},
};

static constexpr int kNumEfuseItems =
    sizeof(efuse_items) / sizeof(efuse_items[0]);

void dump_esp32_efuses() {
  for (int i = 0; i < kNumEfuseItems; i++) {
    const EfuseItem& item = efuse_items[i];

    // Find total bits in all fragments.
    int bit_count = 0;
    for (const esp_efuse_desc_t** p = item.desc; *p != nullptr; p++) {
      bit_count += (*p)->bit_count;
    }

    // Read bits into value.
    uint64_t value = 0;
    assert(bit_count <= sizeof(value) * 8);
    esp_err_t ret =
        esp_efuse_read_field_blob(item.desc, &value, sizeof(value) * 8);
    ESP_ERROR_CHECK(ret);

    // Dump
    const int num_hex_digits = (bit_count + 3) / 4;
    // Note the dynamic field width '*' which is controlled by 
    // num_hex_digits.
    ESP_LOGI(TAG, "%-21s (%d bits) %0*llx", item.name, bit_count,
        num_hex_digits, value);
  }
}

}  // namespace util