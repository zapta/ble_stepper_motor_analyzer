
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

namespace util {

// Done in increments of RTOS ticks (10ms)
inline void delay_ms(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

// Time in RTOS ticks (100Hz)
inline uint32_t rtos_ticks() { return xTaskGetTickCount(); }

// Returned millis are in increments of RTOS tickes (10ms).
inline uint32_t time_ms() { return pdTICKS_TO_MS(xTaskGetTickCount()); }

// Do not enable in normal operation. Blocks interrupts.
void dump_tasks();

// Required for NVS config and for BT.
void nvs_init();

// App build version info.
const char* app_version_str();

}  // namespace util