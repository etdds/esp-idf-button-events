#pragma once

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include <cstring>
#include <utility>

#include "esp_event.h"
#include "esp_system.h"

namespace ButtonEvents {

  /**
   * @brief Possible button states.
   */
  enum class State {
    PRESSED,      ///< Button is pressed. For non-inverted pins, this is logical low.
    NOT_PRESSED,  ///< Button is not pressed. For non-inverted pins, this is logical high.
  };

  /**
   * @brief Types of button events which can occur and be registered.
   */
  enum class EventType {
    BUTTON_UP,          ///< Button transitions from pressed to not pressed state.
    BUTTON_DOWN,        ///< Button transitions from not pressed to pressed state.
    BUTTON_PRESS,       ///< Button 'short' press event.
    BUTTON_LONG_PRESS,  ///< Button 'long' press event.
    BUTTON_HELD,        ///< Button held event. Initial and repeat events use this index.
  };

  /**
   * @brief Forward declaration of the Event manager.
   */
  class EventManager;

  /**
   * @brief Forward declaration of the button builder.
   */
  class ButtonBuilder;

  class Button {
   public:
    /**
     * @brief Create a new button.
     * @details This function returns a utility class which can be used to modify the parameters
     * of the created button. A handler (pointer) to the button is returned after the last
     * parameter has been modified.
     * @warning Buttons are constructed on the heap and remain initialised until reset.
     * @todo Buttons need a method for being deitialised. Should be handled in the destructor
     * and possible wrapped with smart pointers.
     * @param name The name of the button. The names of each button must be unique.
     * @param pin The pin on which the button resides.
     * @return ButtonBuilder
     */
    static ButtonBuilder create(const char* name, gpio_num_t pin);
    /**
     * @brief Add a handler to be called when the specified event occurs.
     * @param handler The handler to be called.
     * @param arg An argument passed to the event handler.
     * @param event The event to which the handler should be registered.
     */
    void add_handler(esp_event_handler_t handler, void* arg, EventType event);
    /**
     * @brief Get the current state of the button.
     * @return State
     */
    State current_state() const;
    /**
     * @brief Get the time at which the last button transition occured.
     * @return size_t
     */
    size_t last_transition() const;
    /**
     * @brief Get the name of the button.
     * @return const char*
     */
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

  /**
   * @brief Utility class used to construct button objects.
   */
  class ButtonBuilder {
   public:
    /**
     * @brief Construct a new Button Builder.
     * @param name The name of the button.
     * @param pin The GPIO pin on which the button resides.
     */
    ButtonBuilder(const char* name, gpio_num_t pin);
    /**
     * @brief Select if the pin has inverted logic.
     * @param inverted If true, high = pressed, low = not pressed.
     * @return ButtonBuilder&
     */
    ButtonBuilder& inverted(const bool inverted);
    /**
     * @brief Select pull up enable for the pin.
     * @param enable If true, internal pullup resistor is enabled.
     * @return ButtonBuilder&
     */
    ButtonBuilder& pull_up(const bool enable);
    /**
     * @brief Select pull down enable for the pin.
     * @param enable If true, internal pulldown resistor is enabled.
     * @return ButtonBuilder&
     */
    ButtonBuilder& pull_down(const bool enable);
    /**
     * @brief Set the debounce time for the pin.
     * @details The debounce time is the duration between when the first edge
     * transition on a pin is detected, and when the pin state is read.
     * @param ms Debounce time in ms.
     * @return ButtonBuilder&
     */
    ButtonBuilder& debounce_ms(const size_t ms);
    /**
     * @brief Set the short press duration.
     * @details This is the minimum duration between the press and not pressed states
     * in order for a press event to be generated.
     * @param ms Press time in ms.
     * @return ButtonBuilder&
     */
    ButtonBuilder& short_press_ms(const size_t ms);
    /**
     * @brief Set the long press duration.
     * @details This is the minimum duration between the press and not pressed states
     * in order for a long press event to be generated. Shorter durations register as a
     * short press.
     * @param ms Long press time in ms.
     * @return ButtonBuilder&
     */
    ButtonBuilder& long_press_ms(const size_t ms);
    /**
     * @brief Set the hold press duration.
     * @details This is the minimum duration required between the initial pressed state
     * and when a BUTTON_HELD event occurs.
     * @param ms The duration requried in ms.
     * @return ButtonBuilder&
     */
    ButtonBuilder& hold_press_ms(const size_t ms);
    /**
     * @brief Set the hold press repeat event duration.
     * @details This is the duration between repeat BUTTON_HELD event generations, after
     * the initial event is generated.
     * @param ms The duration in ms.
     * @return ButtonBuilder&
     */
    ButtonBuilder& hold_repeat_ms(const size_t ms);

    /**
     * @brief Implicitly converts the builder class to a button.
     * @return Button*
     */
    operator Button*() {
      _button->_pin_init(_pull_up, _pull_down);
      return std::move(_button);
    }

   private:
    Button* _button;
    bool _pull_up;
    bool _pull_down;
  };

  /**
   * @brief Converts event data passed to button event handlers to a useful form.
   */
  class EventData {
   public:
    /**
     * @brief Construct a new Event Data object from event handler raw pointer.
     * @param event_data Event data received in event handler.
     */
    explicit EventData(void* event_data) { std::memcpy(this, event_data, sizeof(*this)); }
    EventData() : button(nullptr), timestamp(0), event(EventType::BUTTON_PRESS) {}
    /**
     * @brief Pointer to the button on which the event occured.
     */
    Button* button;
    /**
     * @brief Timestamp in microseconds, at which the event occured.
     */
    uint64_t timestamp;
    /**
     * @brief The type of event which occured.
     */
    EventType event;
  };

  /**
   * @brief Convert milliseconds to microseconds.
   *
   * @param ms Milliseconds to convert
   * @return constexpr size_t microseconds
   */
  constexpr size_t ms_to_us(const size_t ms) { return ms * 1000; }
}  // namespace ButtonEvents
