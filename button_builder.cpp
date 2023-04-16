#include <esp_idf_button_events/esp_idf_button_events.hpp>

#include "esp_system.h"

constexpr size_t ms_to_us(const size_t ms) {
  return ms * 1000;
}

ButtonBuilder::ButtonBuilder(const char* name, gpio_num_t pin) : _button{new Button(name, pin)}, _pull_up(true), _pull_down(false) {
}

ButtonBuilder& ButtonBuilder::inverted(const bool inverted) {
  _button->_inverted = inverted;
  return *this;
}

ButtonBuilder& ButtonBuilder::pull_up(const bool enable) {
  this->_pull_up = enable;
  return *this;
}

ButtonBuilder& ButtonBuilder::pull_down(const bool enable) {
  this->_pull_down = enable;
  return *this;
}

ButtonBuilder& ButtonBuilder::debounce_ms(const size_t ms) {
  _button->_debounce = ms_to_us(ms);
  return *this;
}

ButtonBuilder& ButtonBuilder::short_press_ms(const size_t ms) {
  _button->_short_press = ms_to_us(ms);
  return *this;
}

ButtonBuilder& ButtonBuilder::long_press_ms(const size_t ms) {
  _button->_long_press = ms_to_us(ms);
  return *this;
}

ButtonBuilder& ButtonBuilder::hold_press_ms(const size_t ms) {
  _button->_hold_press = ms_to_us(ms);
  return *this;
}

ButtonBuilder& ButtonBuilder::hold_repeat_ms(const size_t ms) {
  _button->_hold_repeat = ms_to_us(ms);
  return *this;
}