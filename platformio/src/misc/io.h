
#pragma once

// #include <hal/nrf_gpio.h>
#include <stdint.h>

#include "driver/gpio.h"

// #include <zephyr.h>
// #include <misc/button.h>

namespace io {

// TODO: Move to another file

// inline void delay_ms( uint32_t ms) {
//     vTaskDelay(pdMS_TO_TICKS(ms));
// }

// --- Output pins

class OutputPin {
 public:
  // Note: Constructor is not called. Seems to be related to the
  // fact we use this as a global var.
  OutputPin() {}

  void init(gpio_num_t gpio_num, uint32_t initial_state) {
    gpio_num_ = gpio_num;
    gpio_set_direction(gpio_num_, GPIO_MODE_OUTPUT);
    write(initial_state);
  }

  inline void set() { write(1); }
  inline void clear() { write(0); }
  inline void toggle() { write(last_value_ ? 0 : 1); }
  // Value should be 0 or 1.
  inline void write(uint32_t val) {
    last_value_ = val;
    //printf("gpio%d -> %u\n", gpio_num_, val);
    gpio_set_level(gpio_num_, val);
  }
  inline gpio_num_t gpio_num() { return gpio_num_; }

 private:
  // Initialized by init().
  gpio_num_t gpio_num_ = GPIO_NUM_NC;
  uint32_t last_value_ = 0;
};

// Active low.
extern OutputPin LED1;
// extern OutputPin LED2;

// // Output pulses for diagnostics.
// extern OutputPin TIMER_OUT_PIN;
// extern OutputPin ISR_OUT_PIN;

// Enables power on 0 value.
// extern OutputPin<9> SENSOR_POWER_PIN;

// --- Output pins

// class InputPin {
//  public:
//   InputPin() {}
//   void init(uint32_t pin_num, nrf_gpio_pin_pull_t pull) {
//     pin_num_ = pin_num;
//     nrf_gpio_cfg_input(pin_num_, pull);
//   }
//   inline bool read() { return nrf_gpio_pin_read(pin_num_); }
//   inline bool is_high() { return read(); }
//   inline bool is_low() { return !read(); }
//   inline uint32_t pin_num() { return pin_num_; }

//  private:
//   // Initialized by init()
//   uint32_t pin_num_ = 0;
// };

// // Active low. This pin is also the default DFU switch pin
// // of MCUboot.
// extern InputPin SWITCH1;

// // Used to select hardware configuration based on population
// // of resistors R11, R12. An installed resistor results in
// // '0' at the respective input.
// extern InputPin HARDWARE_CFG1;
// extern InputPin HARDWARE_CFG2;

void setup();

// // Read hardware configuration.
// // Returns:
// // 3 - R11, R12, not installed.
// // 2 - Only R11 installed.
// // 1 - Only R12 installed.
// // 0 - Both R11, R12 installed.
// extern uint8_t read_hardware_config();

}  // namespace io