// #include <string.h>

#include "esp_event.h"
// #include "esp_log.h"
#include "esp_system.h"
// #include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "misc/io.h"
#include "misc/util.h"

// #include "lwip/err.h"
// #include "lwip/sys.h"
// #include "nvs_flash.h"

#include <stdio.h>

#include "driver/gpio.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#define STORAGE_NAMESPACE "storage"

#define TAG "main"

void test_nvs() {
  nvs_handle_t my_handle;
  esp_err_t err;

  // Open
  printf("Opening...\n");
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  ESP_ERROR_CHECK(err);
  printf("Opened.\n");

  // if (err != ESP_OK) {
  // }
  // return err;

  // Read
  printf("Reading...\n");
  int32_t offset_v1 = 0;  // value will default to 0, if not set yet in NVS
  err = nvs_get_i32(my_handle, "offset_v1", &offset_v1);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    ESP_ERROR_CHECK(err);
  }
  printf("Read %d\n", offset_v1);

  // Write
  offset_v1++;
  printf("Wrting %d ...\n", offset_v1);

  err = nvs_set_i32(my_handle, "offset_v1", offset_v1);
  ESP_ERROR_CHECK(err);
  printf("Written\n");

  // Commit written value.
  // After setting any values, nvs_commit() must be called to ensure changes are
  // written to flash storage. Implementations may write to storage at other
  // times, but this is not guaranteed.
  printf("Comitting...\n");

  err = nvs_commit(my_handle);
  ESP_ERROR_CHECK(err);
  printf("Commited\n");

  // Close
  printf("Closing...\n");

  nvs_close(my_handle);
  printf("Closed\n");

  // return ESP_OK;
}

// class A {
//  public:
//   A(int val) : val_(val) {}
//   int val_;
// };

// A a1(11);
// int b = 123;

void my_main() {
  // for (;;) {
  //   A a2(22);
  //   printf("a1.val_=%d, a2.val_=%d, b=%d\n", a1.val_, a2.val_, b);
  //   sys_delay_ms(1000);
  // }

  printf("Initializing...\n");
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    printf("Need to create...\n");

    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  printf("Storage ok\n");

  // io::setup();
  for (;;) {
    // sys_delay_ms(1000);

    io::LED1.toggle();

    // vTaskDelay(pdMS_TO_TICKS(1000));
    util::delay_ms(1000);

    io::LED1.toggle();
    io::LED2.toggle();

    util::delay_ms(1000);

    printf("Switch: %d, config: %hhu\n", io::SWITCH1.is_high(), io::read_hardware_config());


    // ESP_LOGI(TAG, "uuhello world");
    // printf("xprintf\n");
    // printf("\n\n");
    // test_nvs();
  }
}

// The runtime environment expects a "C" main.
extern "C" {
void app_main() { my_main(); }
}