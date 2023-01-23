#pragma once

#include <stdint.h>
#include "acquisition/analyzer.h"

namespace ble_service {

void setup(uint8_t hardware_config, uint16_t adc_ticks_per_amp);

// If state notification is enabled, send a notification with 
// this state.
void notify_state_if_enabled(const analyzer::State& state);

 // Returns true if a host is connected.
bool is_connected();

}  // namespace ble_service