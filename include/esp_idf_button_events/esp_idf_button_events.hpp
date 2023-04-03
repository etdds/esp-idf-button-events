#pragma once

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <stdio.h>

#include <array>

#include "esp_event.h"
#include "esp_event_base.h"
// #include <esp_event_loop.h>

#include "esp_system.h"

class EventManager;

enum class State {
  PRESSED,
  NOT_PRESSED,
};

enum {
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_PRESS,
  BUTTON_LONG_PRESS,
  BUTTON_HELD,
};

class Button {
 public:
  explicit Button(gpio_num_t pin);
  // explicit Button(gpio_num_t pin, bool inverted);
  // explicit Button(gpio_num_t pin, bool inverted, size_t long_press, size_t short_press);

  void add_handler(esp_event_handler_t handler, void* arg, int32_t event);

  gpio_num_t pin;
  bool debounce;
  bool inverted;
  uint32_t event_shift;
  State state;
  uint64_t down_time;
  EventGroupHandle_t _event_group;
  EventManager& _manager;
  esp_timer_handle_t debounce_timer;
  esp_timer_handle_t press_timer;
  esp_event_base_t event_base;
};

class ButtonBuilder {};

class EventManager {
 public:
  static EventManager& instance();
  // TODO
  size_t add_button(Button* b);

 private:
  friend class Button;
  EventManager();
  void task_loop();
  EventGroupHandle_t _event_group;
  // esp_event_base_t get_event_base();
  // esp_event_loop_handle_t event_loop;
  size_t _button_count;
  std::array<Button*, 2> buttons;
  esp_event_loop_handle_t loop_with_task;
};