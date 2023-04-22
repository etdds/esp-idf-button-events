#include "event_manager.hpp"

#include "esp_event.h"

using namespace EventBit;

EventManager& EventManager::instance() {
  static EventManager _instance;
  return _instance;
};

Storage::Binding EventManager::add_button(Button* button) {
  return _buttons.add(button);
}

EventGroupHandle_t EventManager::event_group(const size_t index) {
  return _event_groups[index];
}

void EventManager::add_event(Button* button, EventType event, esp_event_handler_t handler, void* arg) {
  esp_event_handler_instance_register_with(loop_with_task, button->_name, static_cast<int32_t>(event), handler, arg, nullptr);
}

EventManager::EventManager() {
  for(auto& event: _event_groups) {
    event = xEventGroupCreate();
  }

  xTaskCreate(
    [](void* arg) {
      auto manager = static_cast<EventManager*>(arg);
      manager->task_loop();
    },
    // TODO: Kconfig for:
    // - STACK
    // - Priority
    "button_event_manager", configMINIMAL_STACK_SIZE * 4, this, 5, NULL);

  // TODO: Kconfig for:
  // - Queue size
  // - Priroity
  // - Task size
  // - Affinity
  // - Using default event loop vs dedicated
  esp_event_loop_args_t loop_with_task_args = {.queue_size = 5,
                                               .task_name = "loop_task",  // task will be created
                                               .task_priority = uxTaskPriorityGet(NULL),
                                               .task_stack_size = 3072,
                                               .task_core_id = tskNO_AFFINITY};

  ESP_ERROR_CHECK(esp_event_loop_create(&loop_with_task_args, &loop_with_task));
};

void EventManager::_send_event(Button* button, EventType event) {
  auto e = EventData();
  e.button = button;
  e.timestamp = esp_timer_get_time();
  e.event = event;
  esp_event_post_to(loop_with_task, button->_name, static_cast<uint32_t>(event), &e, sizeof(e), portMAX_DELAY);
}

void EventManager::task_loop() {
  // TODO: Refactor me. Have an event to handler mapping, rather than if, else if.
  while(true) {
    // TODO need to be able to handle multiple sets of event loops to support more than (6) buttons.
    EventBits_t bits = xEventGroupWaitBits(_event_groups[0], 0xFFFFFF, pdTRUE, pdFALSE, portMAX_DELAY);

    auto events = EventBit::Generator(bits);
    for(const auto& event: events) {
      auto button = _buttons[event.button];
      if(event.trigger == Trigger::PRESS_EVENT) {
        if(!button->_debounce_active) {
          button->_debounce_active = true;
          esp_timer_start_once(button->_debounce_timer, button->_debounce);
        }
      }

      if(event.trigger == Trigger::TIMER_EVENT) {
        button->_debounce_active = false;
        if(button->_current_state == State::PRESSED) {
          button->_transition_time = esp_timer_get_time();
          esp_timer_start_once(button->_held_timer, button->_hold_press);
          // TODO: Kconfig to disable transition events
          _send_event(button, EventType::BUTTON_DOWN);
        }
        else {
          auto current_time = esp_timer_get_time();
          auto delta_us = (current_time - button->_transition_time);
          esp_timer_stop(button->_held_timer);
          // TODO: Kconfig to disable transition events
          _send_event(button, EventType::BUTTON_UP);

          if(delta_us > button->_long_press) {
            _send_event(button, EventType::BUTTON_LONG_PRESS);
          }
          else if(delta_us > button->_short_press) {
            _send_event(button, EventType::BUTTON_PRESS);
          }
          else {
            // No press
          }
        }
      }
      if(event.trigger == Trigger::REPEAT_EVENT) {
        // TODO, maybe make repeat events selectable.
        _send_event(button, EventType::BUTTON_HELD);
      }
    }
  }
}