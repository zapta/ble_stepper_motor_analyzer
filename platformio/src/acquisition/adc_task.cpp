

#include "adc_task.h"

#include <stdio.h>
#include "driver/adc.h"
#include "esp_assert.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "io/io.h"
#include "sdkconfig.h"

// Should be equivalent to sizeof(*adc_digi_output_data_t)
#define BYTES_PER_VALUE 2

// Number of pairs of samples per a read packet.
#define VALUE_PAIRS_PER_BUFFER 64

#define VALUES_PER_BUFFER (2 * VALUE_PAIRS_PER_BUFFER)

// Number of buffer bytes per a read packet.  Having
// a value of 256 here seems to work well. Increasing this to
// let's say 800x4 causes in stability (with IDF 4.4.3).
#define BYTES_PER_BUFFER (VALUES_PER_BUFFER * BYTES_PER_VALUE)

// Num of buffer in the ADC/DMA circular queue. We want to have
// a sufficient size to allow backlog in processing.
#define NUM_BUFFERS (2048 / VALUE_PAIRS_PER_BUFFER)

#if !CONFIG_IDF_TARGET_ESP32
#error "Unexpected target CPU."
#endif

static const adc_digi_init_config_t adc_dma_config = {
    .max_store_buf_size = NUM_BUFFERS * BYTES_PER_BUFFER,
    .conv_num_each_intr = BYTES_PER_BUFFER,
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

static uint8_t bytes_buffer[BYTES_PER_BUFFER] = {0};

void adc_test_main() {
  ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));
  ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
  ESP_ERROR_CHECK(adc_digi_start());

  for (;;) {
    io::LED1.clear();
    uint32_t num_ret_bytes = 0;
    esp_err_t err_code = adc_digi_read_bytes(bytes_buffer, BYTES_PER_BUFFER,
                                             &num_ret_bytes, ADC_MAX_DELAY);
    io::LED1.set();

    // Sanity check the results.
    if (err_code != ESP_OK || num_ret_bytes != BYTES_PER_BUFFER) {
      printf("ADC read failed: %0x %u\n", err_code, num_ret_bytes);
      assert(false);
    }

    adc_digi_output_data_t *values = (adc_digi_output_data_t *)&bytes_buffer[0];

    printf("%hu %4hu %hu %4hu\n", values[0].type1.channel, values[0].type1.data,
           values[1].type1.channel, values[1].type1.data);
    uint16_t chan0 = values[0].type1.channel;
    uint16_t chan1 = values[1].type1.channel;
    for (int i = 0; i < VALUE_PAIRS_PER_BUFFER; i += 2) {
      if (values[i].type1.channel != chan0 ||
          values[i + 1].type1.channel != chan1) {
        printf("\n----\n\n");
        for (int j = 0; j < VALUE_PAIRS_PER_BUFFER; j += 2) {
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
