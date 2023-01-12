#pragma once

#include "driver/adc.h"

namespace adc_task {

void setup();

void raw_capture(adc_digi_output_data_t** values, int* values_count);


}  // namespace adc_task