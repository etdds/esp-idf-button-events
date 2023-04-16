#pragma once

#include <cstddef>
#include <esp_idf_button_events/esp_idf_button_events.hpp>

#include "button_storage.hpp"
#include "esp_event.h"
#include "esp_event_base.h"
#include "event_bits.hpp"

// TODO doxygen

class Button;

class EventManager {
 public:
  static EventManager& instance();
  Storage::Binding add_button(Button* button);
  // TODO int32_t event, could use an enum class.
  void add_event(Button* button, int32_t event, esp_event_handler_t handler, void* arg);
  EventGroupHandle_t event_group(const size_t index);

 private:
  EventManager();
  void task_loop();
  Storage::ButtonHandler<Button*, CONFIG_ESP_BE_MAX_BUTTON_COUNT, EventBit::buttons_per_group()> _buttons;
  void _send_event(Button* button, const EventType event);

  // TOOD the plus 1 is ugly, add a function which calculates this.
  std::array<EventGroupHandle_t, (CONFIG_ESP_BE_MAX_BUTTON_COUNT / EventBit::buttons_per_group()) + 1> _event_groups;
  esp_event_loop_handle_t loop_with_task;
};

// TODO support more buttons.
static_assert(CONFIG_ESP_BE_MAX_BUTTON_COUNT <= 6, "No support for more than six buttons. Event groups need additions.");
