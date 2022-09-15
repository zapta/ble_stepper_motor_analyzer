#include "debounced_input.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr uint32_t SETTLING_TIME_RTOS_TICKS = pdMS_TO_TICKS(100);

bool DebouncedInput::update(uint32_t rtos_ticks_now) {
  last_in_value_ = in_pin_.is_high();
  // Case 1: pin is same as stable state.
  if (stable_state_ == last_in_value_) {
    changing_ = false;
  }

  // Case 2: pin just starting a transition from a stable state.
  else if (!changing_) {
    changing_ = true;
    change_start_rtos_ticks_ = rtos_ticks_now;
  }

  // Case 3: Transition became stable.
  else if ((rtos_ticks_now - change_start_rtos_ticks_) >=
           SETTLING_TIME_RTOS_TICKS) {
    stable_state_ = !stable_state_;
    changing_ = false;
  }

  // Case 4: Nothing to do.
  else {
    // No change.
  }

  return stable_state_;
}

void DebouncedInput::dump_state() {
  // uint32_t a;
  printf("%u, %d, %d, %d, %u\n", in_pin_.pin_num(), last_in_value_,
         stable_state_, changing_, change_start_rtos_ticks_);
}