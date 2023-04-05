
#pragma once

#include <stdint.h>

// #include "acquisition/analyzer.h"

namespace nvs_config {




struct AcquistionSettings {
  // Offsets to substract from the ADC readings to have zero reading
  // when the current is zero.
  // Typically around ~1900 which represents ~1.5V from the current
  // sensors.
  int16_t offset1;
  int16_t offset2;
  // If true, reverse interpretation of forward/backward movement.
  bool is_reverse_direction;
};

  extern const AcquistionSettings kDefaultAcquisitionSettings;


bool read_acquisition_settings(AcquistionSettings* settings);
bool write_acquisition_settings(const AcquistionSettings& settings);

// Null terminated str. Max len 16 chars.
typedef char BleNickname[17];

struct BleSettings {
  BleNickname nickname;
};

extern const BleSettings kDefaultBleDefaultSetting;

bool read_ble_settings(BleSettings* settings);
bool write_ble_settings(const BleSettings& settings);

}  // namespace nvs_config