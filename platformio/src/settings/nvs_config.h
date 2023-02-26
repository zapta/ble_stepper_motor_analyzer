
#pragma once

#include "acquisition/analyzer.h"

namespace nvs_config {

bool read_acquisition_settings(analyzer::Settings* settings);

bool write_acquisition_settings(const analyzer::Settings& settings);

}  // namespace nvs_config