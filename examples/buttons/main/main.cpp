#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include <esp_idf_button_events/esp_idf_button_events.hpp>

#include "esp_system.h"
static void press_any(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  auto event = EventData(event_data);
  ESP_LOGI("MAIN", "Any handler: %s: ID %li", event.button->name(), id);
}

static void press_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  auto event = EventData(event_data);
  ESP_LOGI("MAIN", "Press handler: %s: ID %li", event.button->name(), id);
}

static void long_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  auto event = EventData(event_data);
  ESP_LOGI("MAIN", "Long handler: %s: ID %li", event.button->name(), id);
}

// Main function
extern "C" void app_main(void) {
  Button* b = Button::create("Button A", GPIO_NUM_0);
  Button* b1 = Button::create("Button B", GPIO_NUM_35);

  uint32_t value = 55;
  uint32_t value_any = 65;

  b1->add_handler(press_any, &value_any, BUTTON_DOWN);
  b1->add_handler(press_any, &value_any, BUTTON_UP);
  b1->add_handler(press_any, &value_any, BUTTON_PRESS);
  b1->add_handler(press_any, &value_any, BUTTON_LONG_PRESS);
  b1->add_handler(press_any, &value_any, BUTTON_HELD);

  b1->add_handler(press_handler, &value_any, BUTTON_PRESS);
  b1->add_handler(long_handler, &value_any, BUTTON_LONG_PRESS);

  b->add_handler(long_handler, &value, BUTTON_LONG_PRESS);
  b->add_handler(press_handler, &value, BUTTON_PRESS);

  while(1) {
    // Delay the task for 1000ms (1 second)
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}