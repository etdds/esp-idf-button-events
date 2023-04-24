#pragma once

#include <cstddef>
#include <esp_idf_button_events/button.hpp>

#include "button_storage.hpp"
#include "esp_event.h"
#include "esp_event_base.h"
#include "event_bits.hpp"

namespace ButtonEvents {
  /**
   * @brief Calculates the number of event groups requrired for a givent button count.
   * @return constexpr size_t Event group count.
   */
  constexpr size_t event_group_count() { return (CONFIG_ESP_BE_MAX_BUTTON_COUNT / EventBit::buttons_per_group()) + 1; }

  /**
   * @brief Internally used component class for managing button events and propogating them to subscribers.
   */
  class EventManager {
   public:
    /**
     * @brief Get the singleton class instance. The event manager is initialised the first time this is called.
     * @return EventManager&
     */
    static EventManager& instance();
    /**
     * @brief Add a button to be managed by the event manager. Buttons are referenced by their storage address.
     *        Duplicate additions of the same address are ignored.
     * @param button The button to add.
     * @return Storage::Binding Parameters used to set the button's binding to the event manager.
     */
    Storage::Binding add_button(Button* button);

    /**
     * @brief Connects an event for a button to a handler.
     *
     * @param button The button to which the event is tied.
     * @param event  The type of event.
     * @param handler The handler called when the event occurs.
     * @param arg An argument passed to the event handler.
     */
    void add_event(Button* button, EventType event, esp_event_handler_t handler, void* arg);

    /**
     * @brief Get the event group handler at a given index.
     * @param index The index to fetch.
     * @return EventGroupHandle_t
     */
    EventGroupHandle_t event_group(const size_t index);

   private:
    EventManager();
    void task_loop();
    Storage::ButtonHandler<Button*, CONFIG_ESP_BE_MAX_BUTTON_COUNT, EventBit::buttons_per_group()> _buttons;
    void _send_event(Button* button, EventType event);

    std::array<EventGroupHandle_t, event_group_count()> _event_groups;
    esp_event_loop_handle_t loop_with_task;
  };

  // TODO support more buttons.
  static_assert(CONFIG_ESP_BE_MAX_BUTTON_COUNT <= 6, "No support for more than six buttons. Event groups need additions.");

}  // namespace ButtonEvents
