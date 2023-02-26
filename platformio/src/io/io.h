
#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "io/button.h"
#include "io/input_pin.h"
#include "io/output_pin.h"

namespace io {

// LEDs are Active high.
extern OutputPin LED1;
extern OutputPin LED2;

// Output signals for testing only. Not connected
// on the board.
extern OutputPin TEST1;  // io4
extern OutputPin TEST2;  // io16

// Button input is active low.
extern Button BUTTON1;

// Read hardware configuration. Can be called
// only once.
//
// Returns:
// 0 - CFG1, CFG2 resistors, not installed.
// 1 - Only CFG1 resistor installed.
// 2 - Only CFG2 resistor installed.
// 3 - Both CXG1, CFG2 resistors installed.
extern uint8_t read_hardware_config();

}  // namespace io