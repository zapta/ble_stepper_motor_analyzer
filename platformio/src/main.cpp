// #include <string.h>

#include "acquisition/adc_task.h"
#include "acquisition/analyzer.h"
#include "esp_event.h"
#include "misc/elapsed.h"
// #include "esp_log.h"
#include "esp_system.h"
// #include "esp_wifi.h"
#include "ble/ble_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "io/io.h"
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

static analyzer::State state;

static analyzer::AdcCaptureBuffer capture_buffer;

void my_main() {
  printf("main started.\n");
  analyzer::Settings settings = {.offset1 = 2000 - 188,
                                 .offset2 = 2000 - 230,
                                 .is_reverse_direction = false};
  analyzer::setup(settings);
  adc_task::setup();

  // For testing. To detect restarts.
  printf("\n*** Press BUTTON to start...\n");
  for (;;) {
    const Button::ButtonEvent button_event = io::BUTTON1.update();
    if (button_event == Button::EVENT_SHORT_CLICK) {
      break;
    }
    vTaskDelay(10);
  }

  // Dump raw capture.
  // for (int iter = 0;; iter++) {
  //   adc_digi_output_data_t* values;
  //   int count;
  //   adc_task::raw_capture(&values, &count);
  //   // printf("\n---\n");
  //   for (int i = 0; i < count; i += 2) {
  //     const adc_digi_output_data_t& v1 = values[i];
  //     const adc_digi_output_data_t& v2 = values[i + 1];
  //     // printf("%d, %d, %4d, %4u, %4u\n", v1.type1.channel, v2.type1.channel,
  //     //        i / 2, v1.type1.data, v2.type1.data);
  //     printf("%hu, %hu\n",
  //            v1.type1.data, v2.type1.data);
  //   }
  //   printf("\n");
  //   vTaskDelay(100);
  // }

  for (int iter = 0;; iter++) {
    // Blocking. 50Hz.
    analyzer::pop_next_state(&state);

    if ((iter & 0x0f) == 0) {
      io::LED1.toggle();
    }

    // Dump state
    if (iter % 100 == 0) {
      analyzer::dump_state(state);
      adc_task::dump_stats();
    }

    // Dump capture buffer
    // if (iter % 150 == 5) {
    //   analyzer::get_last_capture_snapshot(&capture_buffer);
    //   for (int i = 0; i < capture_buffer.items.size(); i++) {
    //     const analyzer::AdcCaptureItem* item = capture_buffer.items.get(i);
    //     printf("%hd, %hd\n", item->v1, item->v2);
    //   }
    //   vTaskDelay(100);
    // }
  }

  return;

  // ble_service::test();

  // printf("Initializing...\n");
  // esp_err_t err = nvs_flash_init();
  // if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
  //     err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
  //   printf("Need to create...\n");

  //   // NVS partition was truncated and needs to be erased
  //   // Retry nvs_flash_init
  //   ESP_ERROR_CHECK(nvs_flash_erase());
  //   err = nvs_flash_init();
  // }
  // ESP_ERROR_CHECK(err);
  // printf("Storage ok\n");

  //   // vTaskDelay(pdMS_TO_TICKS(1000));
}

// The runtime environment expects a "C" main.
extern "C" void app_main() { my_main(); }
