#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include <esp_idf_button_events/esp_idf_button_events.hpp>

#include "esp_system.h"

// Main function
extern "C" void app_main(void) {
  while(1) {
    // Delay the task for 1000ms (1 second)
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}