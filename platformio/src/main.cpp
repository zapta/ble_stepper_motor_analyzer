
#include <stdio.h>

#include "acquisition/adc_task.h"
#include "acquisition/analyzer.h"
#include "ble/ble_service.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "io/io.h"
#include "misc/elapsed.h"
#include "misc/util.h"
// #include "nvs.h"
// #include "nvs_flash.h"
#include "settings/controls.h"
#include "settings/nvs.h"

static constexpr auto TAG = "main";

static const analyzer::Settings kDefaultSettings = {
    .offset1 = 1800, .offset2 = 1800, .is_reverse_direction = false};

static analyzer::State state;

static analyzer::AdcCaptureBuffer capture_buffer;

static void setup() {
  // Set initial LEDs values.
  io::LED1.clear();
  io::LED2.clear();

  // Init config.
  nvs::setup();

  // Fetch settings.
  analyzer::Settings settings;
  if (!nvs::read_acquisition_settings(&settings)) {
    ESP_LOGE(TAG, "Failed to read settings, will use default.");
    settings = kDefaultSettings;
  }
  ESP_LOGI(TAG, "Settings: %d, %d, %d", settings.offset1, settings.offset2,
           settings.is_reverse_direction);

  // Init acquisition.
  analyzer::setup(settings);
  adc_task::setup();
}

static uint32_t loop_counter = 0;

static void loop() {
  loop_counter++;

  Button::ButtonEvent event = io::BUTTON1.update();
  if (event == Button::EVENT_LONG_PRESS) {
    controls::zero_calibration();
  } else if (event == Button::EVENT_SHORT_CLICK) {
    controls::toggle_direction(nullptr);
  }

  // Blocking. 50Hz.
  analyzer::pop_next_state(&state);

  if ((loop_counter & 0x0f) == 0) {
    io::LED1.toggle();
  }

  // Dump state
  // if (loop_counter % 100 == 0) {
  //   analyzer::dump_state(state);
  //   adc_task::dump_stats();
  // }

  // Dump capture buffer
  if (loop_counter % 150 == 5) {
    analyzer::get_last_capture_snapshot(&capture_buffer);
    for (int i = 0; i < capture_buffer.items.size(); i++) {
      const analyzer::AdcCaptureItem* item = capture_buffer.items.get(i);
      printf("%hd,%hd\n", item->v1, item->v2);
    }
  }
}

// The runtime environment expects a "C" main.
extern "C" void app_main() {
  setup();
  for (;;) {
    loop();
  }
}
