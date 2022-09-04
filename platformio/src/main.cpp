// #include <string.h>

#include "esp_event.h"
// #include "esp_log.h"
#include "esp_system.h"
// #include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "lwip/err.h"
// #include "lwip/sys.h"
// #include "nvs_flash.h"

#define TAG "xyz"

void my_main() {
  for (;;) {
    sys_delay_ms(500);
    // ESP_LOGI(TAG, "uuhello world");
    printf("printf\n");
  }
}

// The runtime environment expects a "C" main.
extern "C" {
void app_main() { my_main(); }
}