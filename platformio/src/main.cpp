
#include <stdio.h>

#include "acquisition/acq_consts.h"
#include "acquisition/adc_task.h"
#include "acquisition/analyzer.h"
#include "ble/ble_host.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "io/io.h"
#include "misc/efuses.h"
#include "misc/elapsed.h"
#include "misc/util.h"
#include "settings/controls.h"
#include "settings/nvs_config.h"
#include "tools/enum_code_gen.h"

static constexpr auto TAG = "main";

static analyzer::State state;

static analyzer::AdcCaptureBuffer capture_buffer;

// Used to blink N times LED 2.
static Elapsed led2_timer;
// Down counter. If value > 0, then led2 blinks and bit 0 controls
// the led state.
static uint16_t led2_counter;

static Elapsed periodic_timer;

// Used to generate blink to indicates that
// acquisition is working.
static uint32_t analyzer_counter = 0;

static void start_led2_blinks(uint16_t n) {
  led2_timer.reset();
  led2_counter = n * 2;
  io::LED2.write(led2_counter > 0);
}

// Determine hardware configuration based on configuration
// resistors.
static uint16_t determine_adc_ticks_per_amp(uint8_t hardware_config) {
  switch (hardware_config) {
    case 0:
      // CFG1, CFG2 resistors not installed.
      return acq_consts::CC6920BSO5A_ADC_TICKS_PER_AMP;
      break;
    case 1:
      // Only CFG1 resistors is installed.
      return acq_consts::TMCS1108A4B_ADC_TICKS_PER_AMP;
      break;
    // Configurations 0, 1 are reserved.
    default:
      ESP_LOGE(TAG, "Unexpected hardware config %hhu", hardware_config);
      return 0;
  }
}

static void setup() {
  // Set initial LEDs values.
  io::LED1.clr();
  io::LED2.clr();

  ESP_LOGI(TAG, "Version: [%s]", util::app_version_str());

  // For diagnostics.
  efuses::dump_esp32_efuses();

  // Init nvs. Used also by ble_host.
  util::nvs_init();

  // Fetch acquisition settings.
  nvs_config::AcquistionSettings settings;
  if (!nvs_config::read_acquisition_settings(&settings)) {
    ESP_LOGE(TAG, "Failed to read acquisition settings, will use default.");
    settings = nvs_config::kDefaultAcquisitionSettings;
  }
  ESP_LOGI(TAG, "Acqusition settings: %d, %d, %d", settings.offset1,
      settings.offset2, settings.is_reverse_direction);

  // Init acquisition.
  analyzer::setup(settings);
  adc_task::setup();

  // Determine the hardware confiuration to pass to ble host.
  const uint8_t hardware_config = io::read_hardware_config();
  ESP_LOGI(TAG, "Hardware config: %hhu", hardware_config);
  const uint16_t adc_ticks_per_amp =
      determine_adc_ticks_per_amp(hardware_config);
  if (!adc_ticks_per_amp) {
    ESP_LOGE(TAG, "Invalid hardware config: %hhu", hardware_config);
    assert(0);
  }
  ESP_LOGI(TAG, "ADC ticks per Amp: %hu", adc_ticks_per_amp);

  // Initialize ble host.
  ble_host::setup(hardware_config, adc_ticks_per_amp);
}

static bool is_connected = false;

static void loop() {
  // Handle button.
  const Button::ButtonEvent button_event = io::BUTTON1.update();
  if (button_event != Button::EVENT_NONE) {
    ESP_LOGI(TAG, "Button event: %d", button_event);

    // Handle single click. Reverse direction.
    if (button_event == Button::EVENT_SHORT_CLICK) {
      bool new_is_reversed_direcction;
      const bool ok = controls::toggle_direction(&new_is_reversed_direcction);
      const uint16_t num_blinks = !ok ? 10 : new_is_reversed_direcction ? 2 : 1;
      start_led2_blinks(num_blinks);
    }

    // Handle long press. Zero calibration.
    else if (button_event == Button::EVENT_LONG_PRESS) {
      // zero_setting = true;
      const bool ok = controls::zero_calibration();
      start_led2_blinks(ok ? 3 : 10);
    }
  }

  // Update is_connected periodically.
  if (periodic_timer.elapsed_millis() >= 500) {
    periodic_timer.reset();
    is_connected = ble_host::is_connected();
  }

  // Update LED blinks.  Blinking indicates analyzer works
  // and provides states. High speed blink indicates connection
  // status.
  const int blink_shift = is_connected ? 0 : 3;
  const bool blink_state = ((analyzer_counter >> blink_shift) & 0x7) == 0x0;
  // Supress LED1 while blinking LED2. We want to have them appart on the
  // board such that they don't interfere visually.
  io::LED1.write(blink_state && !led2_counter);

  if (led2_counter > 0 && led2_timer.elapsed_millis() >= 500) {
    led2_timer.reset();
    led2_counter--;
    io::LED2.write(led2_counter > 0 && !(led2_counter & 0x1));
  }

  // Blocking. 50Hz.
  analyzer::pop_next_state(&state);

  analyzer_counter++;
  ble_host::notify_state_if_enabled(state);

  // Dump ADC state
  if (analyzer_counter % 100 == 0) {
    analyzer::dump_state(state);
    // adc_task::dump_stats();
  }
}

// The runtime environment expects a "C" main.
extern "C" void app_main() {
  // Use this to updated the ESPIDF enum names tables.
  // enum_code_gen::gen_tables_code();

  setup();
  for (;;) {
    loop();
  }
}
