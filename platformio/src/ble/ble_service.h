#pragma once

#include <stdint.h>

namespace ble_service {

void setup(uint8_t hardware_config, uint16_t adc_ticks_per_amp);

void notify();

}  // namespace ble_service