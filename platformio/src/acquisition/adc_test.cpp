/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "driver/adc.h"

#include <stdio.h>
#include <string.h>

#include "esp_assert.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "io/io.h"
#include "sdkconfig.h"

// Should be equivalent to sizeof(*adc_digi_output_data_t)
#define BYTES_PER_VALUE 2

// Number of pairs of samples per a read packet.
#define VALUE_PAIRS_PER_BUFFER 64

#define VALUES_PER_BUFFER (2 * VALUE_PAIRS_PER_BUFFER)

// #define TIMES 256

// constexpr int x =  sizeof(int);

// Number of buffer bytes per a read packet.
#define BYTES_PER_BUFFER (VALUES_PER_BUFFER * BYTES_PER_VALUE)

// #define BYTES_PER_BUFFER (PAIRS_PER_BUFFER * 2 * 2)

// Minimum 2
#define NUM_BUFFERS 32

// #define GET_UNIT(x) ((x >> 3) & 0x1)

// #if CONFIG_IDF_TARGET_ESP32

// #define ADC_RESULT_BYTE 2

// For ESP32, this should always be set to 1
// #define ADC_CONV_LIMIT_EN 1
// ESP32 only supports ADC1 DMA mode
// #define ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1
// #define ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE1

// #elif CONFIG_IDF_TARGET_ESP32S2
// #error "Reched"
// #define ADC_RESULT_BYTE 2
// #define ADC_CONV_LIMIT_EN 0
// #define ADC_CONV_MODE ADC_CONV_BOTH_UNIT
// #define ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2

// #elif CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32H2
// #error "Reched"
// #define ADC_RESULT_BYTE 4
// #define ADC_CONV_LIMIT_EN 0
// #define ADC_CONV_MODE ADC_CONV_ALTER_UNIT  // ESP32C3 only supports alter
// mode #define ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2

// #elif CONFIG_IDF_TARGET_ESP32S3
// #error "Reched"
// #define ADC_RESULT_BYTE 4
// #define ADC_CONV_LIMIT_EN 0
// #define ADC_CONV_MODE ADC_CONV_BOTH_UNIT
// #define ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2

// #endif

// #if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3 ||
//     CONFIG_IDF_TARGET_ESP32H2
// #error "Reched"
// static uint16_t adc1_chan_mask = BIT(2) | BIT(3);
// static uint16_t adc2_chan_mask = BIT(0);
// static adc_channel_t channel[3] = {ADC1_CHANNEL_2, ADC1_CHANNEL_3,
//                                    (ADC2_CHANNEL_0 | 1 << 3)};
// #endif

// #if CONFIG_IDF_TARGET_ESP32S2
// #error "Reched"
// static uint16_t adc1_chan_mask = BIT(2) | BIT(3);
// static uint16_t adc2_chan_mask = BIT(0);
// static adc_channel_t channel[3] = {ADC1_CHANNEL_2, ADC1_CHANNEL_3,
//                                    (ADC2_CHANNEL_0 | 1 << 3)};
// #endif
// #if CONFIG_IDF_TARGET_ESP32
// // static constexpr uint16_t adc1_chan_mask = BIT(6) | BIT(7);
// // static constexpr uint16_t adc2_chan_mask = 0;
// // static adc_channel_t channel[1] = {ADC1_CHANNEL_7};
// // static constexpr adc_channel_t channel[] = {ADC_CHANNEL_6, ADC_CHANNEL_7};
// #endif

// static const char *TAG = "ADC DMA";

static const adc_digi_init_config_t adc_dma_config = {
    //.max_store_buf_size = 1024,
    .max_store_buf_size = NUM_BUFFERS * BYTES_PER_BUFFER,
    // Max bytes per read.
    // .conv_num_each_intr = TIMES,
    .conv_num_each_intr = BYTES_PER_BUFFER,
    .adc1_chan_mask = BIT(6) | BIT(7),
    .adc2_chan_mask = 0,
};

// TODO: Ok to have only two entries in this array?
static adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {
    {
        .atten = ADC_ATTEN_DB_0,
        .channel = 6,
        .unit = 0,   ///ADC_UNIT_1,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
    },
    {
        .atten = ADC_ATTEN_DB_0,
        .channel = 7,
        .unit = 0, // ADC_UNIT_1,
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
    },
};

static const adc_digi_configuration_t dig_cfg = {
    .conv_limit_en = 1,
    .conv_limit_num = 200,
    // .conv_limit_num = 255,

    .pattern_num = 2,

    // NOTE:  adc_pattern are filled in below in code.

    .adc_pattern = adc_pattern,

    // 20k to 2M
    // .sample_freq_hz = 10 * 1000,

    // 40k sample pairs per sec.
    // ESP32 range is 611Hz ~ 83333Hz
    .sample_freq_hz = 80 * 1000,
    .conv_mode = ADC_CONV_SINGLE_UNIT_1,
    .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,

    //.pattern_num =
    //.adc_pattern =
};

// static void continuous_adc_init(  // const uint16_t adc1_chan_mask,
//                                   // const uint16_t adc2_chan_mask,
//                                   // const adc_channel_t *channel,
//                                   // const uint8_t channel_num
// ) {
//   // adc_digi_init_config_t adc_dma_config = {
//   //     .max_store_buf_size = 1024,
//   //     .conv_num_each_intr = TIMES,
//   //     .adc1_chan_mask = adc1_chan_mask,
//   //     .adc2_chan_mask = adc2_chan_mask,
//   // };
//   // printf("Initializing ADC\n");
//   // ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));
//   ESP_ERROR_CHECK(adc_digi_initialize(&xadc_dma_config));
//   // printf("ADC initialized\n");

//   // adc_digi_configuration_t dig_cfg = {
//   //     .conv_limit_en = ADC_CONV_LIMIT_EN,
//   //     //.conv_limit_num = 250,
//   //     .conv_limit_num = 255,

//   //     .pattern_num = channel_num,

//   //     // NOTE:  adc_pattern are filled in below in code.

//   //     .adc_pattern = nullptr,

//   //     // 20k to 2M
//   //     // .sample_freq_hz = 10 * 1000,
//   //     .sample_freq_hz = 20 * 1000,
//   //     .conv_mode = ADC_CONV_MODE,
//   //     .format = ADC_OUTPUT_TYPE,

//   //     //.pattern_num =
//   //     //.adc_pattern =
//   // };

//   // adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};

//   // TODO(zapta): Change the size to 2 (num of channels)
//   // adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {};

//   // dig_cfg.pattern_num = channel_num;

//   // for (int i = 0; i < channel_num; i++) {
//   //   uint8_t unit = GET_UNIT(channel[i]);
//   //   uint8_t ch = channel[i] & 0x7;
//   //   adc_pattern[i].atten = ADC_ATTEN_DB_0;
//   //   adc_pattern[i].channel = ch;
//   //   adc_pattern[i].unit = unit;
//   //   adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

//   //   ESP_LOGI(TAG, "adc_pattern[%d].atten is :%x", i,
//   adc_pattern[i].atten);
//   //   ESP_LOGI(TAG, "adc_pattern[%d].channel is :%x", i,
//   //   adc_pattern[i].channel); ESP_LOGI(TAG, "adc_pattern[%d].unit is :%x",
//   i,
//   //   adc_pattern[i].unit);
//   // }
//   // // Arrary pointer reference.
//   // dig_cfg.adc_pattern = adc_pattern;

//   // ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
//   ESP_ERROR_CHECK(adc_digi_controller_configure(&xdig_cfg));
//   // esp_err_t ret = adc_digi_controller_configure(&dig_cfg);
//   // ESP_ERROR_CHECK(ret);
// }

// #if !CONFIG_IDF_TARGET_ESP32
// #error "Reached"
// static bool check_valid_data(const adc_digi_output_data_t *data) {
//   const unsigned int unit = data->type2.unit;
//   if (unit > 2) return false;
//   if (data->type2.channel >= SOC_ADC_CHANNEL_NUM(unit)) return false;

//   return true;
// }
// #endif

// static int dma_errors = 0;

static uint8_t bytes_buffer[BYTES_PER_BUFFER] = {0};

// void xapp_main(void) {

void adc_test_main() {
  // esp_err_t ret;
  // uint32_t ret_num_bytes = 0;

  // uint8_t result_bytes[TIMES] = {0};

  // memset(result_bytes, 0xcc, TIMES);

  // continuous_adc_init(adc1_chan_mask, adc2_chan_mask, channel,
  //                     sizeof(channel) / sizeof(adc_channel_t));
  // continuous_adc_init();

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

    // assert(ret == ESP_OK);
    //  if (ret != ESP_OK) {
    //    dma_errors++;
    //    continue;
    //  }

    // Based on the configuration, we expect to have exactly this
    // number of byes per buffer.
    // assert(num_ret_bytes == BYTES_PER_BUFFER);

    // const uint32_t ret_ret_values = num_ret_bytes / BYTES_PER_VALUE;

    // } || ret == ESP_ERR_INVALID_STATE) {
    //   if (ret == ESP_ERR_INVALID_STATE) {
    /**
     * @note 1
     * Issue:
     * As an example, we simply print the result out, which is super slow.
     * Therefore the conversion is too fast for the task to handle. In this
     * condition, some conversion results lost.
     *
     * Reason:
     * When this error occurs, you will usually see the task watchdog
     * timeout issue also. Because the conversion is too fast, whereas the
     * task calling `adc_digi_read_bytes` is slow. So `adc_digi_read_bytes`
     * will hardly block. Therefore Idle Task hardly has chance to run. In
     * this example, we add a `vTaskDelay(1)` below, to prevent the task
     * watchdog timeout.
     *
     * Solution:
     * Either decrease the conversion speed, or increase the frequency you
     * call `adc_digi_read_bytes`
     */
    // }

    // ESP_LOGI("TASK:", "ret is %x, ret_num is %d", ret, ret_num);

    adc_digi_output_data_t *values = (adc_digi_output_data_t *)&bytes_buffer[0];
    // adc_digi_output_data_t *p1 = p0 +/ 1;

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



    // if (ret != 0x103) {
    //   printf("%x\n", ret);
    // }

    //       for (int i = 0; i < ret_num; i += ADC_RESULT_BYTE) {
    //         // adc_digi_output_data_t *p = (void *)&result[i];
    //         // adc_digi_output_data_t *p = (adc_digi_output_data_t
    //         *)&result[i];
    // #if CONFIG_IDF_TARGET_ESP32
    //         // ESP_LOGI(TAG, "Unit: %d, Channel: %d, Value: %x", 1,
    //         // p->type1.channel,
    //         //          p->type1.data);
    // #else
    // #error "Not reached"
    //         if (ADC_CONV_MODE == ADC_CONV_BOTH_UNIT ||
    //             ADC_CONV_MODE == ADC_CONV_ALTER_UNIT) {
    //           if (check_valid_data(p)) {
    //             ESP_LOGI(TAG, "Unit: %d,_Channel: %d, Value: %x",
    //             p->type2.unit + 1,
    //                      p->type2.channel, p->type2.data);
    //           } else {
    //             // abort();
    //             ESP_LOGI(TAG, "Invalid data [%d_%d_%x]", p->type2.unit + 1,
    //                      p->type2.channel, p->type2.data);
    //           }
    //         }
    // #if CONFIG_IDF_TARGET_ESP32S2
    // #error "Not reached"
    //         else if (ADC_CONV_MODE == ADC_CONV_SINGLE_UNIT_2) {
    //           ESP_LOGI(TAG, "Unit: %d, Channel: %d, Value: %x", 2,
    //           p->type1.channel,
    //                    p->type1.data);
    //         } else if (ADC_CONV_MODE == ADC_CONV_SINGLE_UNIT_1) {
    //           ESP_LOGI(TAG, "Unit: %d, Channel: %d, Value: %x", 1,
    //           p->type1.channel,
    //                    p->type1.data);
    //         }
    // #endif  // #if CONFIG_IDF_TARGET_ESP32S2
    // #error "Not reached"
    // #endif
    //       }
    // See `note 1`
    // vTaskDelay(1);
    // ESP_ERROR_CHECK(esp_task_wdt_reset());

    // } else if (ret == ESP_ERR_TIMEOUT) {
    //   /**
    //    * ``ESP_ERR_TIMEOUT``: If ADC conversion is not finished until
    //    Timeout,
    //    * you'll get this return error. Here we set Timeout ``portMAX_DELAY``,
    //    so
    //    * you'll never reach this branch.
    //    */
    //   ESP_LOGW(TAG, "No data, increase timeout or reduce
    //   conv_num_each_intr"); vTaskDelay(1000);
    // }
  }

  // adc_digi_stop();
  // ret = adc_digi_deinitialize();
  // assert(ret == ESP_OK);
}
