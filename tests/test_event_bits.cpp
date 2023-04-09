#include <event_bits.hpp>

#include "doctest.h"

using namespace EventBit;

TEST_CASE("Event default constructor sets values to defaults") {
  Event event;
  CHECK(event.trigger == Trigger::NO_EVENT);
  CHECK(event.group_index == 0);
  CHECK(event.button == 0);
}

TEST_CASE("Event equality operator works correctly") {
  Event event1;
  Event event2;
  Event event3;
  Event event4;
  Event event5;

  SUBCASE("Testing on trigger") {
    CHECK(event1 == event2);
    event2.trigger = Trigger::TIMER_EVENT;
    CHECK(event1 != event2);
  }

  SUBCASE("Testing on group index") {
    CHECK(event1 == event3);
    event3.group_index = 2;
    CHECK(event1 != event3);
  }

  SUBCASE("Testing on button") {
    CHECK(event1 == event4);
    event4.button = 1;
    CHECK(event1 != event4);
  }

  SUBCASE("Testing on multiple") {
    CHECK(event1 == event5);
    event5.button = 2;
    event5.trigger = Trigger::PRESS_EVENT;
    event5.group_index = 6;
    CHECK(event1 != event5);
  }
}

TEST_CASE("get_bit_mask returns correct value") {
  SUBCASE("No event") {
    CHECK(get_bit_mask(Trigger::NO_EVENT, 0) == 0x0);
    CHECK(get_bit_mask(Trigger::NO_EVENT, 5) == 0x0);
  }
  SUBCASE("Press event") {
    CHECK(get_bit_mask(Trigger::PRESS_EVENT, 1) == Trigger::PRESS_EVENT << 3);
    CHECK(get_bit_mask(Trigger::PRESS_EVENT, 2) == Trigger::PRESS_EVENT << 6);
    CHECK(get_bit_mask(Trigger::PRESS_EVENT, 8) == Trigger::PRESS_EVENT << 0);
    CHECK(get_bit_mask(Trigger::PRESS_EVENT, 15) == Trigger::PRESS_EVENT << 21);
  }

  SUBCASE("Timer event") {
    CHECK(get_bit_mask(Trigger::TIMER_EVENT, 0) == Trigger::TIMER_EVENT << 0);
    CHECK(get_bit_mask(Trigger::TIMER_EVENT, 4) == Trigger::TIMER_EVENT << 12);
    CHECK(get_bit_mask(Trigger::TIMER_EVENT, 6) == Trigger::TIMER_EVENT << 18);
  }

  SUBCASE("Repeat event") {
    CHECK(get_bit_mask(Trigger::REPEAT_EVENT, 2) == Trigger::REPEAT_EVENT << 6);
    CHECK(get_bit_mask(Trigger::REPEAT_EVENT, 0) == Trigger::REPEAT_EVENT << 0);
  }
}

TEST_CASE("get_group_index returns correct value") {
  CHECK(get_group_index(0) == 0);
  CHECK(get_group_index(1) == 0);
  CHECK(get_group_index(8) == 1);
  CHECK(get_group_index(15) == 1);
  CHECK(get_group_index(16) == 2);
}

TEST_CASE("Generator iterator returns all Events") {
  SUBCASE("Single button") {
    SUBCASE("Single event") {
      Generator generator(0b010);
      auto it = generator.begin();
      CHECK(it->trigger == Trigger::TIMER_EVENT);
      CHECK(it->group_index == 0);
      CHECK(it->button == 0);
      it++;
      CHECK(it == generator.end());
    }
    SUBCASE("Multiple events") {
      Generator generator(0b101);
      auto it = generator.begin();
      CHECK(it->trigger == Trigger::PRESS_EVENT);
      CHECK(it->group_index == 0);
      CHECK(it->button == 0);
      it++;
      CHECK(it->trigger == Trigger::REPEAT_EVENT);
      CHECK(it->group_index == 0);
      CHECK(it->button == 0);
      it++;
      CHECK(it == generator.end());
    }
  }

  SUBCASE("Multiple buttons") {
    SUBCASE("Single event each") {
      Generator generator(0b1100);
      auto it = generator.begin();
      CHECK(it->trigger == Trigger::REPEAT_EVENT);
      CHECK(it->group_index == 0);
      CHECK(it->button == 0);
      it++;
      CHECK(it->trigger == Trigger::PRESS_EVENT);
      CHECK(it->group_index == 0);
      CHECK(it->button == 1);
      it++;
      CHECK(it == generator.end());
    }

    SUBCASE("Multiple event each") {
      Generator generator(0b11101);
      auto it = generator.begin();
      CHECK(it->trigger == Trigger::PRESS_EVENT);
      CHECK(it->group_index == 0);
      CHECK(it->button == 0);
      it++;
      CHECK(it->trigger == Trigger::REPEAT_EVENT);
      CHECK(it->group_index == 0);
      CHECK(it->button == 0);
      it++;
      CHECK(it->trigger == Trigger::PRESS_EVENT);
      CHECK(it->group_index == 0);
      CHECK(it->button == 1);
      it++;
      CHECK(it->trigger == Trigger::TIMER_EVENT);
      CHECK(it->group_index == 0);
      CHECK(it->button == 1);
      it++;
      CHECK(it == generator.end());
    }
  }
  SUBCASE("Second group") {
    SUBCASE("Single event") {
      Generator generator(0b010, 1);
      auto it = generator.begin();
      CHECK(it->trigger == Trigger::TIMER_EVENT);
      CHECK(it->group_index == 1);
      CHECK(it->button == 8);
      it++;
      CHECK(it == generator.end());
    }
  }

  SUBCASE("Range based for loop") {
    Generator generator(0b1001001001, 0);
    size_t button = 0;
    for(const auto& event: generator) {
      CHECK(event.trigger == Trigger::PRESS_EVENT);
      CHECK(event.group_index == 0);
      CHECK(event.button == button++);
    }
    // Four events should have occurred.
    CHECK(button == 4);
  }
}

TEST_CASE("Higher event bits are not used.") {
  uint32_t bits = 0xFF000000;
  Generator generator(bits);
  auto it = generator.begin();
  CHECK(it == generator.end());
}