
#pragma once

// #include <hal/nrf_gpio.h>
#include <stdint.h>

// #include "driver/gpio.h"
#include "freertos/task.h"


// #include <zephyr.h>
// #include <misc/button.h>

namespace util {

// TODO: Move to another file

inline void delay_ms( uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}


}  // namespace io