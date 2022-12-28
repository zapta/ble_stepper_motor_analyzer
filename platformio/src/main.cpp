// #include <string.h>

#include "esp_event.h"
// #include "esp_log.h"
#include "esp_system.h"
// #include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "io/io.h"
#include "misc/util.h"
#include "ble/ble_service.h"

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

  ble_service::test();

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

  for (;;) {
    // Yields to avoid starvations and WDT trigger.
    util::delay_ms(10);
    Button::ButtonEvent event = io::BUTTON1.update();
    
    io::LED1.write(io::BUTTON1.is_pressed());
    if (event != Button::EVENT_NONE) {
      printf("Event: %d\n", event);
    }

    // sys_delay_ms(1000);

    // io::LED1.toggle();

    // vTaskDelay(pdMS_TO_TICKS(1000));
    // util::delay_ms(1000);

    // io::LED1.toggle();
    // io::LED2.toggle();

    // util::delay_ms(1000);

    // uint64_t t0 = util::time_us();
    // uint32_t ms = util::time_ms();
    // uint64_t t1 = util::time_us();
    // printf("dt=%u %u\n", (uint32_t)(t1 - t0), ms);
  }
}

// The runtime environment expects a "C" main.
extern "C" void app_main() { my_main(); }
