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
  return level ^ inverted ? State::NOT_PRESSED : State::PRESSED;
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

void Button::timer_held_callback(void* arg) {
  auto b = static_cast<Button*>(arg);
  esp_timer_start_once(b->_held_timer, b->_hold_repeat);
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

  static bool __attribute__((unused)) once = []() {
    gpio_install_isr_service(0);
    return true;
  }();

  gpio_isr_handler_add(_pin, Button::button_isr_handler, this);
}

Button::Button(const char* name, gpio_num_t pin) :
  _pin(pin),
#ifdef CONFIG_ESP_BE_DEFAULT_BUTTON_INVERTED
  _inverted(true),
#else
  _inverted(false),
#endif
  _name(name),
  _debounce(ms_to_us(CONFIG_ESP_BE_DEFAULT_DEBOUNCE_MS)),
  _short_press(ms_to_us(CONFIG_ESP_BE_DEFAULT_SHORT_PRESS_MS)),
  _long_press(ms_to_us(CONFIG_ESP_BE_DEFAULT_LONG_PRESS_MS)),
  _hold_press(ms_to_us(CONFIG_ESP_BE_DEFAULT_HELD_MS)),
  _hold_repeat(ms_to_us(CONFIG_ESP_BE_DEFAULT_HELD_REPEAT_MS)),
  _current_state{State::NOT_PRESSED},
  _debounce_active{false},
  _transition_time(0) {
  auto binding = Manager().add_button(this);
  assert(binding.valid);

  // TODO Timer naming could somehow also have the button name and type.
  // Fow now, both timers have the same name.
  esp_timer_create_args_t timer_param = {.callback = Button::timer_debounce_callback,
                                         .arg = this,
                                         .dispatch_method = ESP_TIMER_TASK,
                                         .name = name,
                                         .skip_unhandled_events = false};


  esp_timer_create(&timer_param, &_debounce_timer);

  timer_param.callback = Button::timer_held_callback;
  timer_param.name = name;
  esp_timer_create(&timer_param, &_held_timer);

  _press_event_bit = get_bit_mask(Trigger::PRESS_EVENT, binding.button_index);
  _timer_event_bit = get_bit_mask(Trigger::TIMER_EVENT, binding.button_index);
  _repeat_event_bit = get_bit_mask(Trigger::REPEAT_EVENT, binding.button_index);
  _event_group = Manager().event_group(binding.event_group_index);
}

void Button::add_handler(esp_event_handler_t handler, void* arg, EventType event) {
  EventManager::instance().add_event(this, event, handler, arg);
}