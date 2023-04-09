
#include "settings/nvs_config.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "io/io.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <memory.h>
#include <rom/crc.h>
#include <stdio.h>

namespace nvs_config {

static constexpr auto TAG = "nvs_config";

static constexpr auto kStorageNamespace = "settings";

const AcquistionSettings kDefaultAcquisitionSettings = {
    .offset1 = 1800, .offset2 = 1800, .is_reverse_direction = false};

const BleSettings kDefaultBleDefaultSetting = {.nickname = ""};

[[nodiscard]] bool read_acquisition_settings(AcquistionSettings* settings) {
  // Open
  nvs_handle_t my_handle = -1;
  esp_err_t err = nvs_open(kStorageNamespace, NVS_READONLY, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "read_acquisition_settings() failed to open nvs: %04x", err);
    return false;
  }

  // Read offset1.
  int16_t offset1;
  err = nvs_get_i16(my_handle, "offset1", &offset1);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "read_acquisition_settings() failed read offset1: %04x", err);
  }

  // Read offset 2.
  int16_t offset2;
  if (err == ESP_OK) {
    err = nvs_get_i16(my_handle, "offset2", &offset2);
    if (err != ESP_OK) {
      ESP_LOGW(
          TAG, "read_acquisition_settings() failed read offset2: %04x", err);
    }
  }

  // Read is_reverse flag.
  uint8_t is_reverse_direction;
  if (err == ESP_OK) {
    err = nvs_get_u8(my_handle, "is_reverse", &is_reverse_direction);
    if (err != ESP_OK) {
      ESP_LOGW(
          TAG, "read_acquisition_settings() failed read is_reverse: %04x", err);
    }
  }

  // Close.
  nvs_close(my_handle);

  // Handle results.
  if (err != ESP_OK) {
    return false;
  }
  settings->offset1 = offset1;
  settings->offset2 = offset2;
  settings->is_reverse_direction = (bool)is_reverse_direction;
  return true;
}

[[nodiscard]] bool write_acquisition_settings(const AcquistionSettings& settings) {
  // Open
  nvs_handle_t my_handle = -1;
  esp_err_t err = nvs_open(kStorageNamespace, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "write_acquisition_settings() failed to open nvs: %04x", err);
    return false;
  }

  // TODO: What is the recomanded way to avoid crashing while
  // writing to flash while interrupts are active? Currently we use
  // taskDISABLE_INTERRUPTS, which disables only on the current
  // core, and configure menuconfig for a single core FreeRTOS.
  //
  // Interrupts are disabled here for up to 10ms as measured
  // on osciloscope.

  // Write offset1.
  if (err == ESP_OK) {
    taskDISABLE_INTERRUPTS();
    err = nvs_set_i16(my_handle, "offset1", settings.offset1);
    taskENABLE_INTERRUPTS();
    if (err != ESP_OK) {
      ESP_LOGE(TAG,
          "write_acquisition_settings() failed to write offset1: %04x", err);
    }
  }

  // Write offset2.
  if (err == ESP_OK) {
    taskDISABLE_INTERRUPTS();
    err = nvs_set_i16(my_handle, "offset2", settings.offset2);
    taskENABLE_INTERRUPTS();
    if (err != ESP_OK) {
      ESP_LOGE(TAG,
          "write_acquisition_settings() failed to write offset2: %04x", err);
    }
  }

  // Write is_reverse flag.
  if (err == ESP_OK) {
    taskDISABLE_INTERRUPTS();
    err = nvs_set_u8(
        my_handle, "is_reverse", settings.is_reverse_direction ? 0 : 1);
    taskENABLE_INTERRUPTS();
    if (err != ESP_OK) {
      ESP_LOGE(TAG,
          "write_acquisition_settings() failed to write is_reverse: %04x", err);
    }
  }

  // Commit updates.
  if (err == ESP_OK) {
    taskDISABLE_INTERRUPTS();
    err = nvs_commit(my_handle);
    taskENABLE_INTERRUPTS();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "write_acquisition_settings() failed to commit: %04x", err);
    }
  }

  // Close.
  nvs_close(my_handle);
  return err == ESP_OK;
}

[[nodiscard]] bool write_ble_settings(const BleSettings& settings) {
  // Open
  nvs_handle_t my_handle = -1;
  esp_err_t err = nvs_open(kStorageNamespace, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "write_ble_settings() failed to open nvs: %04x", err);
    return false;
  }

  // TODO: What is the recomanded way to avoid crashing while
  // writing to flash while interrupts are active? Currently we use
  // taskDISABLE_INTERRUPTS, which disables only on the current
  // core, and configure menuconfig for a single core FreeRTOS.
  //
  // Interrupts are disabled here for up to 10ms as measured
  // on osciloscope.

  // Verify that the string contains a null terminator.
  const int len = strlen(settings.nickname);
  if (len >= sizeof(settings.nickname)) {
    ESP_LOGE(TAG, "write_ble_settings() nickname too long: %d", len);
    return false;
  }

  // Write nickname.
  if (err == ESP_OK) {
    taskDISABLE_INTERRUPTS();
    err = nvs_set_str(my_handle, "ble_nickname", settings.nickname);
    taskENABLE_INTERRUPTS();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "write_ble_settings() failed to write nickname: %04x", err);
    } else {
      ESP_LOGI(
          TAG, "write_ble_settings() nickname written [%s]", settings.nickname);
    }
  }

  // Commit updates.
  if (err == ESP_OK) {
    taskDISABLE_INTERRUPTS();
    err = nvs_commit(my_handle);
    taskENABLE_INTERRUPTS();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "write_ble_settings() failed to commit: %04x", err);
    } else {
      ESP_LOGI(TAG, "write_ble_settings() commit OK");
    }
  }

  // Close.
  nvs_close(my_handle);
  return err == ESP_OK;
}

[[nodiscard]] bool read_ble_settings(BleSettings* settings) {
  // Open
  nvs_handle_t my_handle = -1;
  esp_err_t err = nvs_open(kStorageNamespace, NVS_READONLY, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "read_ble_settings() failed to open nvs: %04x", err);
    return false;
  }

  // Read nickname string. Note that 'size' is an input/output argument.
  size_t size = sizeof(settings->nickname);
  err =
      nvs_get_str(my_handle, "ble_nickname", (char*)&settings->nickname, &size);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "read_ble_settings() failed read nickname: %04x", err);
  }

  // Close.
  nvs_close(my_handle);
  return (err == ESP_OK);
}

}  // namespace nvs_config