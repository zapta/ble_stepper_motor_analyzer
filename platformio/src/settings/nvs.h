
#pragma once

#include "acquisition/analyzer.h"

namespace nvs {

void setup();

bool read_acquisition_settings(analyzer::Settings* settings);

bool write_acquisition_settings(const analyzer::Settings& settings);

}  // namespace nvs