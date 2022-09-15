
#pragma once

#include <stdint.h>
#include "driver/gpio.h"

// #include <zephyr.h>
// #include <misc/button.h>

namespace io {

// --- Output pins

class OutputPin {
 public:
  OutputPin( gpio_num_t gpio_num, uint32_t initial_state) 
  : gpio_num_(gpio_num) {
    gpio_set_direction(gpio_num_, GPIO_MODE_OUTPUT);
    write(initial_state);
  }

  inline void set() { write(1); }
  inline void clear() { write(0); }
  inline void toggle() { write(last_value_ ? 0 : 1); }
  // Value should be 0 or 1.
  inline void write(uint32_t val) {
    last_value_ = val;
    gpio_set_level(gpio_num_, val);
  }
  inline gpio_num_t gpio_num() { return gpio_num_; }

 private:
  const gpio_num_t gpio_num_ = GPIO_NUM_NC;
  // Used to toggle since we can't read output pins(?).
  uint32_t last_value_ = 0;
};

extern OutputPin LED1;
extern OutputPin LED2;



// --- Input pins

class InputPin {
 public:
  InputPin(gpio_num_t gpio_num, gpio_pull_mode_t pull_mode)
  :gpio_num_(gpio_num) {
    // pin_num_ = pin_num;
        gpio_set_direction(gpio_num_, GPIO_MODE_INPUT);
        gpio_set_pull_mode(gpio_num_, pull_mode);
  }
  inline bool read() { return gpio_get_level(gpio_num_); }
  inline bool is_high() { return read(); }
  inline bool is_low() { return !read(); }

 private:
  const gpio_num_t gpio_num_;
};

// Active low.
extern InputPin SWITCH1;

// Read hardware configuration.
// Returns:
// 0 - CFG1, CFG2, not installed.
// 1 - Only CFG1 installed.
// 2 - Only CFG2 installed.
// 3 - Both CXG1, CFG2 installed.
extern uint8_t read_hardware_config();

}  // namespace io