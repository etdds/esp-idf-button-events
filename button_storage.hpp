#pragma once

#include <array>
#include <cstddef>

namespace Storage {
  /**
   * @brief Represents a button's binding to the event manger.
   */
  struct Binding {
    bool valid;                ///< Set true if the binding was valid. i.e enough space in container.
    size_t button_index;       ///< The index the button is assigned.
    size_t event_group_index;  ///< The event group index the button should receive.
  };

  /// @brief The storage container for button handlers. Stores handles and issues bindings on request.
  /// @tparam ptr_type The type of pointer for a butotn.
  /// @tparam max_button_count  The maximum number of buttons to store.
  /// @tparam buttons_per_group  The number of buttons on a single event group.
  template<typename ptr_type, size_t max_button_count, size_t buttons_per_group>
  class ButtonHandler {
   public:
    using storage = std::array<ptr_type, max_button_count>;
    ButtonHandler() : _storage{{nullptr}} {}

    /**
     * @brief Add an handler to be managed by the container.
     *
     * @param item The handler to add.
     * @return Binding A binding describing the handler position and characteristics in the container.
     */
    Binding add(ptr_type item) {
      size_t index = 0;
      for(auto& entry: _storage) {
        if(entry == nullptr) {
          entry = item;
          return {true, index, index / buttons_per_group};
        }
        index++;
      }
      return {false, 0, 0};
    }

    /**
     * @brief Remove a hander by the container.
     *
     * @param item A previosly added handler
     * @return true The handler was removed.
     * @return false The hander wasn't found in the container, and not removed.
     */
    bool remove(ptr_type item) {
      for(auto& entry: _storage) {
        if(entry == item) {
          entry = nullptr;
          return true;
        }
      }
      return false;
    }

    /**
     * @brief Query if the container is empty.
     *
     * @return true The container is empty.
     * @return false The container is not empty
     */
    bool empty() const { return size() == 0; }

    /**
     * @brief Query if the container is full.
     *
     * @return true The container is full
     * @return false The container is not full.
     */
    bool full() const { return size() == max_button_count; }

    /**
     * @brief Query the number of handlers added to the container.
     *
     * @return size_t The number of handlers
     */
    size_t size() const {
      size_t count = 0;
      for(auto& entry: _storage) {
        if(entry != nullptr) {
          count++;
        }
      }
      return count;
    }

    /**
     * @brief Overload operator to access items by index
     *
     * @param index The index to access
     * @return ptr_type The handler at that index
     */
    ptr_type operator[](const size_t index) { return _storage[index]; }

   private:
    storage _storage;
  };
}  // namespace Storage