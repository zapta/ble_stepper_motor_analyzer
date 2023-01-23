#pragma once

#include <stdint.h>
#include "acquisition/analyzer.h"

namespace ble_service {

void setup(uint8_t hardware_config, uint16_t adc_ticks_per_amp);

void notify_state_if_enabled(const analyzer::State& state);

}  // namespace ble_service