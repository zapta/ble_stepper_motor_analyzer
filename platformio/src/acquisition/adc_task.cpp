

#include "adc_task.h"

#include <stdio.h>

#include "driver/adc.h"
#include "esp_assert.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "io/io.h"
#include "sdkconfig.h"
#include "misc/elapsed.h"

namespace adc_task { 

// Should be equivalent to sizeof(*adc_digi_output_data_t)
static constexpr uint32_t _BYTES_PER_VALUE = 2;

// Number of pairs of samples per a read packet.
static constexpr uint32_t _VALUE_PAIRS_PER_BUFFER = 200;

static constexpr uint32_t _VALUES_PER_BUFFER = 2 * _VALUE_PAIRS_PER_BUFFER;

// Number of buffer bytes per a read packet.  
// Increasing this value 'too much' causes in stability (with IDF 4.4.3).
static constexpr uint32_t _BYTES_PER_BUFFER = _VALUES_PER_BUFFER * _BYTES_PER_VALUE;


// Num of buffer in the ADC/DMA circular queue. We want to have
// a sufficient size to allow backlog in processing.
static constexpr uint32_t _NUM_BUFFERS = 2048 / _VALUE_PAIRS_PER_BUFFER;

#if !CONFIG_IDF_TARGET_ESP32
#error "Unexpected target CPU."
#endif

static const adc_digi_init_config_t adc_dma_config = {
    .max_store_buf_size = _NUM_BUFFERS * _BYTES_PER_BUFFER,
    .conv_num_each_intr = _BYTES_PER_BUFFER,
    .adc1_chan_mask = BIT(6) | BIT(7),
    .adc2_chan_mask = 0,
};

// TODO: Ok to have only two entries in this array instead
// of SOC_ADC_PATT_LEN_MAX?
static adc_digi_pattern_config_t adc_pattern[] = {
    {
        .atten = ADC_ATTEN_DB_0,
        .channel = 6,
        .unit = 0,  /// ADC_UNIT_1,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
    },
    {
        .atten = ADC_ATTEN_DB_0,
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

static uint8_t bytes_buffer[_BYTES_PER_BUFFER] = {0};

void adc_task(void *ignored) {

  bool in_order = false;
  uint32_t order_changes = 0;
  uint32_t buffers_count = 0;
  Elapsed timer;

  for (;;) {
    io::LED1.clear();
    uint32_t num_ret_bytes = 0;
    esp_err_t err_code = adc_digi_read_bytes(bytes_buffer, _BYTES_PER_BUFFER,
                                             &num_ret_bytes, ADC_MAX_DELAY);
    io::LED1.set();

    // Sanity check the results.
    if (err_code != ESP_OK || num_ret_bytes != _BYTES_PER_BUFFER) {
      printf("ADC read failed: %0x %u\n", err_code, num_ret_bytes);
      assert(false);
    }

    adc_digi_output_data_t *values = (adc_digi_output_data_t *)&bytes_buffer;

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

    if (timer.elapsed_millis() >= 1000) {
      timer.reset();
      printf("ADC: %s, %u, %u\n", (in_order? "(6,7)" : "(7,6)"), order_changes, buffers_count);
    }

    // printf("%hu %hu %4hu %4hu\n", values[0].type1.channel,
    //        values[1].type1.channel, values[0].type1.data,
    //        values[1].type1.data);

    // uint16_t chan0 = values[0].type1.channel;
    // uint16_t chan1 = values[1].type1.channel;

    // We expect the buffer to have the same order of pairs.
    for (int i = 0; i < _VALUE_PAIRS_PER_BUFFER; i += 2) {
      bool ok = (values[i].type1.channel == (in_order ? 6 : 7)) &&
                (values[i + 1].type1.channel == (in_order ? 7 : 6));
      if (!ok) {
        printf("\n----\n\n");
        for (int j = 0; j < _VALUE_PAIRS_PER_BUFFER; j += 2) {
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
