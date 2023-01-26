#pragma once

#include <stdint.h>

namespace acq_consts {

// ADC ticks per 1A current per current sensor type.
// Thease are measured value on the device since they depend
// also output resistance of the sensor and input resistance
// of the ADC.
constexpr uint16_t CC6920BSO5A_ADC_TICKS_PER_AMP = 340;
constexpr uint16_t TMCS1108A4B_ADC_TICKS_PER_AMP = 496;

// How many time the pair of channels is sampled per second.
// This time ticks are used as the data time base.
constexpr uint32_t kTimeTicksPerSec = 40000;

// Number of histogram buckets, each bucket represents
// a band of step speeds.
constexpr int kNumHistogramBuckets = 25;

// Each histogram bucket represents a speed range of 100
// steps/sec, starting from zero. Overflow speeds are
// aggregated in the last bucket.
const int kBucketStepsPerSecond = 200;

}  // namespace acq_consts