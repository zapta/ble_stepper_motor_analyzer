#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace ble2 {

/* Attributes State Machine */
enum {
  IDX_SVC,
  IDX_CHAR_A,
  IDX_CHAR_VAL_A,
  IDX_CHAR_CFG_A,

  IDX_CHAR_B,
  IDX_CHAR_VAL_B,

  IDX_CHAR_C,
  IDX_CHAR_VAL_C,

  HRS_IDX_NB,
};

void setup();

void notify();

}  // namespace ble2