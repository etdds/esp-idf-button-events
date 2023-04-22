#pragma once

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include <array>
#include <cstring>

#include "esp_event.h"
#include "esp_system.h"


enum class State {
  PRESSED,
  NOT_PRESSED,
};

enum class EventType {
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_PRESS,
  BUTTON_LONG_PRESS,
  BUTTON_HELD,
};

class EventManager;
class ButtonBuilder;

class Button {
 public:
  static ButtonBuilder create(const char* name, gpio_num_t pin);
  void add_handler(esp_event_handler_t handler, void* arg, EventType event);
  State current_state() const;
  size_t last_transition() const;
  const char* name() const;

 private:
  Button(const char* name, gpio_num_t pin);
  friend class ButtonBuilder;

  // Button characteristics
  gpio_num_t _pin;
  bool _inverted;
  const char* _name;

  size_t _debounce;
  size_t _short_press;
  size_t _long_press;
  size_t _hold_press;
  size_t _hold_repeat;

  void _pin_init(const bool pull_up, const bool pull_down);

  // Common ISR and timer expired events.
  static void button_isr_handler(void* arg);
  static void timer_debounce_callback(void* arg);
  static void timer_held_callback(void* arg);

  // Button interaction with event manager
  friend class EventManager;
  uint32_t _press_event_bit;
  uint32_t _timer_event_bit;
  uint32_t _repeat_event_bit;
  EventGroupHandle_t _event_group;

  // Internal button state.
  State _current_state;
  bool _debounce_active;
  uint64_t _transition_time;
  esp_timer_handle_t _debounce_timer;
  esp_timer_handle_t _held_timer;
};

class ButtonBuilder {
 public:
  ButtonBuilder(const char* name, gpio_num_t pin);
  ButtonBuilder& inverted(const bool inverted);
  ButtonBuilder& pull_up(const bool enable);
  ButtonBuilder& pull_down(const bool enable);
  ButtonBuilder& debounce_ms(const size_t ms);
  ButtonBuilder& short_press_ms(const size_t ms);
  ButtonBuilder& long_press_ms(const size_t ms);
  ButtonBuilder& hold_press_ms(const size_t ms);
  ButtonBuilder& hold_repeat_ms(const size_t ms);

  operator Button*() {
    _button->_pin_init(_pull_up, _pull_down);
    return std::move(_button);
  }

 private:
  Button* _button;
  bool _pull_up;
  bool _pull_down;
};

class EventData {
 public:
  explicit EventData(void* event_data) { std::memcpy(this, event_data, sizeof(*this)); }
  EventData() : button(nullptr), timestamp(0), event(EventType::BUTTON_PRESS) {}
  Button* button;
  uint64_t timestamp;
  EventType event;
};
