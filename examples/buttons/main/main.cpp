#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include <esp_idf_button_events/esp_idf_button_events.hpp>

#include "esp_system.h"
static void press_any(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  ESP_LOGI("MAIN", "Press ID %li", id);
}

static void press_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  ESP_LOGI("MAIN", "Got press");
}

static void long_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  ESP_LOGI("MAIN", "long press");
}

// Main function
extern "C" void app_main(void) {
  Button b1(GPIO_NUM_0);
  Button b(GPIO_NUM_35);
  b1.add_handler(press_any, NULL, BUTTON_UP);
  b1.add_handler(press_any, NULL, BUTTON_DOWN);
  b1.add_handler(press_any, NULL, BUTTON_PRESS);
  b1.add_handler(press_any, NULL, BUTTON_LONG_PRESS);
  b1.add_handler(press_any, NULL, BUTTON_HELD);

  b.add_handler(press_handler, NULL, BUTTON_PRESS);
  b.add_handler(long_handler, NULL, BUTTON_LONG_PRESS);
  while(1) {
    // Delay the task for 1000ms (1 second)
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}