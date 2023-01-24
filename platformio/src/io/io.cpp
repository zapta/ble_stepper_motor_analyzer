
#include "io.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "misc/util.h"

namespace io {
static constexpr auto TAG = "io";

// Digital outputs.
OutputPin LED1(GPIO_NUM_26, 1);
OutputPin LED2(GPIO_NUM_25, 1);

OutputPin TEST1(GPIO_NUM_2, 0);

// Digital inputs.
static InputPin SWITCH1(GPIO_NUM_27, GPIO_PULLUP_ONLY);
Button BUTTON1(SWITCH1);

static InputPin CFG1(GPIO_NUM_18, GPIO_PULLUP_ONLY);
static InputPin CFG2(GPIO_NUM_19, GPIO_PULLUP_ONLY);

// We allow to read only once, to avoid having conflicting
// values.
static bool hardware_config_read_once = false;

uint8_t read_hardware_config() {
  assert(!hardware_config_read_once);

  // Read the value. Look for three matching reads with a
  // short delay in between to filter out possible noise.
  // Of open pullup inputs.
  uint8_t val = 0;
  int matching_count = 0;
  for (int tries = 0; tries < 10; tries++) {
    util::delay_ms(10);

    // '1' is respective resistor exists.
    const uint8_t new_val =
        (CFG1.is_high() ? 0 : 0x01) | (CFG2.is_high() ? 0 : 0x02);

    // Handle match
    if (new_val == val) {
      // If we have 3 consecutive matches we are good.
      matching_count++;
      if (matching_count >= 3) {
        ESP_LOGI(TAG, "Read hardware config in %d iterations, value: 0x%02hhx",
                 tries + 1, val);
        hardware_config_read_once = true;
        return val;
      };
      continue;
    }

    // Handle no match
    matching_count = 1;
    val = new_val;
  }

  // Couldn't determine a stable value.
  ESP_LOGE(TAG, "Fail to read a stable hardware config: %hhx, %d", val,
           matching_count);
  assert(0);
}

}  // namespace io