#include <esp_idf_button_events/button.hpp>

#include "assert.h"
#include "button_storage.hpp"
#include "event_bits.hpp"
#include "event_manager.hpp"

#define TAG "Event Buttons"

using namespace EventBit;

constexpr auto Manager = EventManager::instance;

ButtonBuilder Button::create(const char* name, gpio_num_t pin) {
  return ButtonBuilder(name, pin);
}

constexpr State to_state(bool level, bool inverted) {
  // TODO: Handle inverted button.
  return level ? State::NOT_PRESSED : State::PRESSED;
}

void IRAM_ATTR Button::button_isr_handler(void* arg) {
  auto b = static_cast<Button*>(arg);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if(!b->_debounce_active) {
    xEventGroupSetBitsFromISR(b->_event_group, b->_press_event_bit, &xHigherPriorityTaskWoken);
  }
  if(xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void Button::timer_debounce_callback(void* arg) {
  auto b = static_cast<Button*>(arg);
  b->_current_state = to_state(gpio_get_level(b->_pin), b->_inverted);
  xEventGroupSetBits(b->_event_group, b->_timer_event_bit);
}

void Button::timer_press_callback(void* arg) {
  auto b = static_cast<Button*>(arg);
  esp_timer_start_once(b->_press_timer, b->_hold_repeat);
  xEventGroupSetBits(b->_event_group, b->_repeat_event_bit);
}

State Button::current_state() const {
  return _current_state;
}

size_t Button::last_transition() const {
  return _transition_time;
}

const char* Button::name() const {
  return _name;
}

void Button::_pin_init(const bool pull_up, const bool pull_down) {
  gpio_config_t io_conf = {
    .pin_bit_mask = 1ULL << _pin,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = (pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE),
    .pull_down_en = (pull_down ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE),
    .intr_type = GPIO_INTR_ANYEDGE,
  };
  gpio_config(&io_conf);

  // TODO get this in a static member function.
  static bool i = false;
  if(!i) {
    gpio_install_isr_service(0);
    i = true;
  }
  gpio_isr_handler_add(_pin, Button::button_isr_handler, this);
}

// TODO: Add KConfig options for configuraiton.
Button::Button(const char* name, gpio_num_t pin) :
  _pin(pin),
  _inverted{false},
  _name(name),
  _debounce(50000),
  _short_press(100000),
  _long_press(3000000),
  _hold_press(5000000),
  _hold_repeat(500000),
  _current_state{State::NOT_PRESSED},
  _debounce_active{false},
  _transition_time(0) {
  auto binding = Manager().add_button(this);
  assert(binding.valid);
  // TODO Timer naming
  esp_timer_create_args_t timer_param = {.callback = Button::timer_debounce_callback,
                                         .arg = this,
                                         .dispatch_method = ESP_TIMER_TASK,
                                         .name = "db_timer",
                                         .skip_unhandled_events = false};


  esp_timer_create(&timer_param, &_debounce_timer);

  // TODO Timer naming.
  timer_param.callback = Button::timer_press_callback;
  timer_param.name = "press_timer";
  esp_timer_create(&timer_param, &_press_timer);

  _press_event_bit = get_bit_mask(Trigger::PRESS_EVENT, binding.button_index);
  _timer_event_bit = get_bit_mask(Trigger::TIMER_EVENT, binding.button_index);
  _repeat_event_bit = get_bit_mask(Trigger::REPEAT_EVENT, binding.button_index);
  _event_group = Manager().event_group(binding.event_group_index);
}

void Button::add_handler(esp_event_handler_t handler, void* arg, int32_t event) {
  EventManager::instance().add_event(this, event, handler, arg);
}