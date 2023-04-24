#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include <esp_idf_button_events/button.hpp>

#include "esp_system.h"

#define LOG_TAG "MAIN"

// Specify namespace to reduce verbosity.
using namespace ButtonEvents;

// Specify a dummy class to demonstrate calling a class member on event handler.
class Object {
 public:
  void handler() { ESP_LOGI(LOG_TAG, "Class handler called"); }
};

// Three event handlers which are called on button events.
static void press_any(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  auto event = EventData(event_data);

  // This would need to be handled better if the argument type was unknown.
  auto arg = static_cast<int*>(handler_args);
  ESP_LOGI(LOG_TAG, "Any handler: %s: ID %li, arg: %d", event.button->name(), id, *arg);
}

static void press_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  auto event = EventData(event_data);
  ESP_LOGI(LOG_TAG, "Press handler: %s: ID %li", event.button->name(), id);
}

static void long_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  auto event = EventData(event_data);
  ESP_LOGI(LOG_TAG, "Long handler: %s: ID %li", event.button->name(), id);
}

extern "C" void app_main(void) {
  // Create a button using the Kconfig default options.
  Button* button_a = Button::create("Button A", GPIO_NUM_0);

  // Create another button, with custom button specific configuration.
  Button* button_b = Button::create("Button B", GPIO_NUM_35)
                       .inverted(false)
                       .pull_up(true)
                       .pull_down(false)
                       .debounce_ms(50)
                       .short_press_ms(100)
                       .long_press_ms(1000)
                       .hold_press_ms(3000)
                       .hold_repeat_ms(200);


  // Example value to pass through to the event handler.
  uint32_t handler_value = 55;

  // An object who's method is called on an event through a lambda.
  Object object;

  // Add handler for all events on button B.
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_DOWN);
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_UP);
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_PRESS);
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_LONG_PRESS);
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_HELD);

  // Add handler specifically press and long events on button B
  button_b->add_handler(press_handler, &handler_value, EventType::BUTTON_PRESS);
  button_b->add_handler(long_handler, &handler_value, EventType::BUTTON_LONG_PRESS);

  // Add press and long handler for button A.
  button_a->add_handler(long_handler, &handler_value, EventType::BUTTON_LONG_PRESS);
  button_a->add_handler(press_handler, &handler_value, EventType::BUTTON_PRESS);

  button_a->add_handler(
    [](void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
      auto o = static_cast<Object*>(handler_args);
      o->handler();
    },
    &object, EventType::BUTTON_PRESS);

  ESP_LOGI(LOG_TAG, "Waiting for events...");
  while(1) {
    // Delay the task for 1000ms (1 second)
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}