
#pragma once

// #include <hal/nrf_gpio.h>
#include <stdint.h>

// #include "driver/gpio.h"
#include "freertos/task.h"

// #include <zephyr.h>
// #include <misc/button.h>

namespace util {

// Done in increments of RTOS ticks (10ms)
inline void delay_ms(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

// Returned millis are in increments of RTOS tickes (10ms).
inline uint32_t time_ms() { return pdTICKS_TO_MS(xTaskGetTickCount()); }

inline uint64_t time_us() { return (uint64_t)esp_timer_get_time(); }

}  // namespace util