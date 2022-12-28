#include "io/button.h"

#include "esp_log.h"
#include "misc/util.h"

static const char* TAG = "Button";

// Max time for a short press.
static constexpr uint32_t T1_RTOS_TICKS = pdMS_TO_TICKS(1000);
// Min time for a long press.
static constexpr uint32_t T2_RTOS_TICKS = pdMS_TO_TICKS(3000);

// #include "misc/io.h"

// namespace button {

// void setup() { BUTTON1.init(&io::SWITCH1); }

// }  // namespace button

Button::ButtonEvent Button::update() {
  const uint32_t rtos_ticks_now = util::rtos_ticks();
  const bool debouncer_val = debounced_in_.update(rtos_ticks_now);

  // Switch is active low.
  const bool is_on = !debouncer_val;

  ButtonEvent result = EVENT_NONE;
  switch (state_) {
    case STATE_RELEASED: {
      if (is_on) {
        state_ = STATE_PRESSED_IDLE;
        time_start_rtos_ticks_ = rtos_ticks_now;
      }
    } break;

    case STATE_PRESSED_IDLE: {
      const uint32_t ticks_in_state = rtos_ticks_now - time_start_rtos_ticks_;
      if (is_on) {
        if (ticks_in_state > T2_RTOS_TICKS) {
          state_ = STATE_PRESSED_LONG;
          result = EVENT_LONG_PRESS;
        }
      } else {
        if (ticks_in_state < T1_RTOS_TICKS) {
          result = EVENT_SHORT_CLICK;
        }
        state_ = STATE_RELEASED;
      }
    } break;

    case STATE_PRESSED_LONG: {
      if (!is_on) {
        state_ = STATE_RELEASED;
        result = EVENT_LONG_RELEASE;
      }
    } break;

    default: {
      ESP_LOGE(TAG, "Unknown button state: %d\n", state_);
      state_ = STATE_RELEASED;
    }
  }

  return result;
}