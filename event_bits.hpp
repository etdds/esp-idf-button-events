#pragma once

#include <cstddef>
#include <iterator>

namespace EventBit {
  /**
   * @brief Enumeration of event triggers which can belong to a single button.
   */
  enum Trigger {
    NO_EVENT = 0x0,     ///< No trigger
    PRESS_EVENT = 0x1,  ///< Button press event, send from ISR.
    TIMER_EVENT = 0x2,  ///< Button debounce timer expired event.
    REPEAT_EVENT = 0x4  ///< Button no state change timer expired event
  };

  /**
   * @brief Get the number of events availalbe in a single bitfield. Value determined by FreeRTOS eventbits.
   *
   * @return constexpr size_t Number of event bits available.
   */
  constexpr size_t event_bit_count() { return 24; }

  /**
   * @brief Get the number of event triggers, execept for no event.
   *
   * @return constexpr size_t Event trigger count
   */
  constexpr size_t event_count() { return 3; }

  /**
   * @brief Get the number of buttons which can be represented in a single event bitfield.
   *
   * @return constexpr size_t Button per bitfield
   */
  constexpr size_t buttons_per_group() { return event_bit_count() / event_count(); }

  /**
   * @brief Sanity check to make sure things are calculated as expected.
   */
  static_assert(buttons_per_group() == 8, "Event bit groups are incorrect!");

  /**
   * @brief Get the bit mask which represents a given trigger for a button.
   * @param event The event trigger to use.
   * @param button_index The button to which the event applies.
   * @return size_t The event bitmask for the trigger on the given button.
   */
  constexpr size_t get_bit_mask(const Trigger event, const size_t button_index) {
    size_t shift = button_index % buttons_per_group();
    return event << (shift * event_count());
  }

  /**
   * @brief Get the group index for a button, considering the maximum number of buttons which can belong in a sinngle event bit mask.
   * @details For example, if 8 buttons can be represented by a single bit mask. Button 8 would belong to index 0, button 9 to index 1.
   *
   * @param button_index The button index to calculate.
   * @return size_t The event group index.
   */
  constexpr size_t get_group_index(const size_t button_index) { return button_index / buttons_per_group(); }

  /**
   * @brief Structure representing a single generated event.
   */
  struct Event {
    Event() {
      trigger = Trigger::NO_EVENT;
      group_index = 0;
      button = 0;
    }

    bool operator==(const Event& other) const {
      auto te = other.trigger == trigger;
      auto be = other.button == button;
      auto ge = other.group_index == group_index;
      return te && be && ge;
    }

    bool operator!=(const Event& other) const {
      auto te = other.trigger == trigger;
      auto be = other.button == button;
      auto ge = other.group_index == group_index;
      return !te || !be || !ge;
    }

    Trigger trigger;     ///< The trigger which caused the event.
    size_t group_index;  ///< The group index to which the event belongs.
    size_t button;       ///< The button on which the event occured.
  };

  /**
   * @brief Class for taking a set of event bits and parsing them as events.
   */
  class Generator {
   public:
    /**
     * @brief Construct a new Generator object
     *
     * @param event_bits A set of event bits used to determing generated events.
     * @param event_group The event group to which the events belonged.
     */
    explicit Generator(size_t event_bits, size_t event_group = 0) :
      _bits(event_bits & ((1 << event_bit_count()) - 1)), _group(event_group) {}

    class iterator {
     public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = Event;
      using difference_type = std::ptrdiff_t;
      using pointer = Event*;
      using reference = Event&;

      explicit iterator(size_t bits, size_t group) : _bits(bits), _group(group), _index(0) { _get_next(); }

      iterator& operator++() {
        _get_next();
        return *this;
      }

      iterator operator++(int) {
        iterator temp(*this);
        _get_next();
        return temp;
      }

      iterator end_iterator() const {
        iterator end_iter(*this);
        end_iter._current.trigger = NO_EVENT;
        end_iter._current.group_index = this->_group;
        end_iter._current.button = buttons_per_group() * (this->_group + 1);
        return end_iter;
      }

      bool operator==(const iterator& other) const { return _current == other._current; }
      bool operator!=(const iterator& other) const { return _current != other._current; }
      reference operator*() { return _current; }
      pointer operator->() { return &_current; }

     private:
      void _get_next() {
        while(!(_bits & 0x1) && _index < event_bit_count()) {
          _bits = _bits >> 1;
          _index++;
        }
        size_t event_offset = (_index % event_count());

        _current = Event();
        _current.button = (_index / event_count()) + (_group * buttons_per_group());
        _current.group_index = _group;
        _current.trigger = static_cast<Trigger>(((_bits << event_offset) & 0x1 << event_offset));

        _bits = _bits >> 1;
        _index++;
      };

      Event _current;
      size_t _bits;
      size_t _group;
      size_t _index;
    };

    /**
     * @brief Get an iterator pointing to the first generated event.
     *
     * @return iterator
     */
    iterator begin() { return iterator(_bits, _group); }
    /**
     * @brief Get an iterrator pointing to the last generated event.
     *
     * @return iterator
     */
    iterator end() { return iterator(_bits, _group).end_iterator(); }

   private:
    size_t _bits;
    size_t _group;
  };

};  // namespace EventBit
