#include "util.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace util {

static char text_buffer[600];

// https://www.freertos.org/uxTaskGetSystemState.html
// https://www.freertos.org/a00021.html#vTaskList
//
// NOTE: This disables interrupts throughout its
// operation. Do no use in normal operation.
void dump_tasks() {
  vTaskList(text_buffer);
  printf("\n");
  printf("Name          State    Prio     Free   ID\n");
  printf("-----------------------------------------\n");
  // NOTE: buffer requires up to 40 chars per task.
  printf("%s\n", text_buffer);
}

}  // namespace util