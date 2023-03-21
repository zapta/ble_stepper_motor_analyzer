#include "util.h"

#include "esp_app_desc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/efuse_hal.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace util {

static constexpr auto TAG = "util";

static char text_buffer[600];

// https://www.freertos.org/uxTaskGetSystemState.html
// https://www.freertos.org/a00021.html#vTaskList
//
// NOTE: This disables interrupts throughout its
// operation. Do no use in normal operation.
void dump_tasks() {
  vTaskList(text_buffer);
  printf("\n");
  printf("Name          State    Prio     Free   ID\n");
  printf("-----------------------------------------\n");
  // NOTE: buffer requires up to 40 chars per task.
  printf("%s\n", text_buffer);
}

void nvs_init() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "nvs_flash_init() ok.");
    return;
  }

  // This is the initial creation in new devices.
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW(TAG, "nvs_flash_init() err 0x%x %s. Erasing nvs...", err,
        esp_err_to_name(err));
    // This should not fail.
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_LOGI(TAG, "nvs erased. Initializing again...");
    // This should not fail.
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI(TAG, "nvs_flash_init() ok.");
    return;
  }

  ESP_LOGE(
      TAG, "nvs_flash_init() fatal error 0x%x %s.", err, esp_err_to_name(err));
  assert(false);
}

// Initialized on first call to app_info_str(). Preusambly from
// main thread on initialization.
static char app_info[110] = "";

// ESP32 0.01 | Mar 20 2023 | 14:27:41  | 75dccb0-dirty | 23aa3d9fb9de4f4a
const char* app_version_str() {
  // Initialize if first call. Assuming a mutex is not necessary.
  if (!app_info[0]) {
    const esp_app_desc_t* app_desc = esp_app_get_description();
    char short_sha[17];
    esp_app_get_elf_sha256(short_sha, sizeof(short_sha));
    const uint32_t revision = efuse_hal_chip_revision();
    snprintf(app_info, sizeof(app_info), "ESP32 %lu.%02lu | %s | %s | %s | %s",
        revision / 100, revision % 100, app_desc->date, app_desc->time,
        app_desc->version, &short_sha[0]);
  }
  
  return app_info;
}

}  // namespace util