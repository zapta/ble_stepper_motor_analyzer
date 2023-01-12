

#include "adc_task.h"

#include <stdio.h>

#include "analyzer_private.h"
#include "driver/adc.h"
#include "esp_assert.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "io/io.h"
#include "misc/elapsed.h"
#include "sdkconfig.h"

// namespace analyzer {
// // Private method of the analyzer.
// void handle_one_sample(const uint16_t raw_v1, const uint16_t raw_v2);
// }  // namespace analyzer.

namespace adc_task {

constexpr uint32_t kBytesPerValue = sizeof(adc_digi_output_data_t);
constexpr uint32_t kValuePairsPerBuffer = 50;
constexpr uint32_t kValuesPerBuffer = 2 * kValuePairsPerBuffer;
constexpr uint32_t kBytesPerBuffer = kValuesPerBuffer * kBytesPerValue;
constexpr uint32_t kNumBuffers = 2048 / kValuePairsPerBuffer;

#if !CONFIG_IDF_TARGET_ESP32
#error "Unexpected target CPU."
#endif

// Raw capture buffer. For testing only.
// NOTE: We assume atomic uint86_t access and don't use a mutex.
constexpr uint32_t kCaptureBuffersCount = 1000 / kValuePairsPerBuffer;
constexpr uint32_t kCaptureValuesCount =
    kCaptureBuffersCount * kValuesPerBuffer;
static adc_digi_output_data_t
    captured_values[kCaptureBuffersCount * kValuePairsPerBuffer];
static uint8_t captured_buffers_count = 0;

void raw_capture(adc_digi_output_data_t **values, int *count) {
  captured_buffers_count = 0;
  while (captured_buffers_count < kCaptureBuffersCount) {
    vTaskDelay(1);
  }
  *values = captured_values;
  *count = kCaptureValuesCount;
}

static const adc_digi_init_config_t adc_dma_config = {
    .max_store_buf_size = kNumBuffers * kBytesPerBuffer,
    .conv_num_each_intr = kBytesPerBuffer,
    .adc1_chan_mask = BIT(6) | BIT(7),
    .adc2_chan_mask = 0,
};

// TODO: Ok to have only two entries in this array instead
// of SOC_ADC_PATT_LEN_MAX?
static adc_digi_pattern_config_t adc_pattern[] = {
    {
        .atten = ADC_ATTEN_DB_11,
        .channel = 6,
        .unit = 0,  /// ADC_UNIT_1,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
    },
    {
        .atten = ADC_ATTEN_DB_11,
        .channel = 7,
        .unit = 0,  // ADC_UNIT_1,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
    },
};

static const adc_digi_configuration_t dig_cfg = {
    .conv_limit_en = 1,
    // .conv_limit_num = 255,
    .conv_limit_num = 200,

    .pattern_num = 2,
    .adc_pattern = adc_pattern,

    // 40k sample pairs per sec.
    // ESP32 range is 611Hz ~ 83333Hz
    .sample_freq_hz = 80 * 1000,
    .conv_mode = ADC_CONV_SINGLE_UNIT_1,
    .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
};

static uint8_t bytes_buffer[kBytesPerBuffer] = {0};

void adc_task(void *ignored) {
  bool in_order = false;
  uint32_t order_changes = 0;
  uint32_t buffers_count = 0;
  uint32_t samples_to_snapshot = 0;
  // Elapsed timer;

  for (;;) {
    io::TEST1.clear();
    uint32_t num_ret_bytes = 0;
    esp_err_t err_code = adc_digi_read_bytes(bytes_buffer, kBytesPerBuffer,
                                             &num_ret_bytes, ADC_MAX_DELAY);
    io::TEST1.set();

    // Sanity check the results.
    if (err_code != ESP_OK || num_ret_bytes != kBytesPerBuffer) {
      printf("ADC read failed: %0x %u\n", err_code, num_ret_bytes);
      assert(false);
    }

    adc_digi_output_data_t *values = (adc_digi_output_data_t *)&bytes_buffer;

    // For testing. Remove after stabilization.
    if (captured_buffers_count < kCaptureBuffersCount) {
      memcpy(&captured_values[captured_buffers_count * kValuesPerBuffer],
             values, kBytesPerBuffer);
      captured_buffers_count++;
    }

    buffers_count++;

    if (values[0].type1.channel == 6) {
      assert(values[1].type1.channel == 7);
      if (!in_order) {
        in_order = true;
        order_changes++;
      }

    } else {
      assert(values[0].type1.channel == 7);
      assert(values[1].type1.channel == 6);
      if (in_order) {
        in_order = false;
        order_changes++;
      }
    }

    // if (timer.elapsed_millis() >= 10000) {
    //   timer.reset();
    //   printf("ADC: %s, [%4u, %4u]  %u, %u\n", (in_order ? "[6,7]" : "[7,6]"),
    //          values[0].type1.data, values[1].type1.data, order_changes,
    //          buffers_count);
    // }

    // printf("%hu %hu %4hu %4hu\n", values[0].type1.channel,
    //        values[1].type1.channel, values[0].type1.data,
    //        values[1].type1.data);

    // We expect the buffer to have the same order of pairs.
    analyzer::enter_mutex();
    {
      for (int i = 0; i < kValuePairsPerBuffer; i += 2) {
        if (in_order) {
          analyzer::isr_handle_one_sample(values[i].type1.data,
                                          values[i + 1].type1.data);
        } else {
          analyzer::isr_handle_one_sample(values[i + 1].type1.data,
                                          values[i].type1.data);
        }

        bool ok = (values[i].type1.channel == (in_order ? 6 : 7)) &&
                  (values[i + 1].type1.channel == (in_order ? 7 : 6));
        if (!ok) {
          printf("\n----\n\n");
          for (int j = 0; j < kValuePairsPerBuffer; j += 2) {
            printf("%3d: %hu %4hu %hu %4hu\n", j, values[j + 0].type1.channel,
                   values[j + 0].type1.data, values[j + 1].type1.channel,
                   values[j + 1].type1.data);
          }

          for (;;) {
            vTaskDelay(1);
          }
        }
      }
    }

    samples_to_snapshot += kValuePairsPerBuffer;
    if (samples_to_snapshot >= 800) {
      analyzer::isr_snapshot_state();
      samples_to_snapshot = 0;
    }

    analyzer::exit_mutex();
  }
}

void setup() {
  ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));
  ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
  ESP_ERROR_CHECK(adc_digi_start());

  TaskHandle_t xHandle = NULL;
  // Create the task, storing the handle.  Note that the passed parameter
  // ucParameterToPass must exist for the lifetime of the task, so in this case
  // is declared static.  If it was just an an automatic stack variable it might
  // no longer exist, or at least have been corrupted, by the time the new task
  // attempts to access it.
  xTaskCreate(adc_task, "ADC", 4000, nullptr, 10, &xHandle);
  configASSERT(xHandle);
}

}  // namespace adc_task
