#include <button_storage.hpp>
#include <cstddef>
#include <cstdint>

#include "doctest.h"

using namespace Storage;

TEST_CASE("Button storage is initially empty") {
  auto storage = ButtonHandler<uint32_t*, 6, 3>();
  CHECK(storage.empty() == true);
  CHECK(storage.full() == false);
  CHECK(storage.size() == 0);
}

TEST_CASE("Adding and removing items") {
  auto storage = ButtonHandler<uint32_t*, 3, 2>();
  std::array<uint32_t, 3> items{0, 1, 2};
  size_t index = 0;

  for(auto& item: items) {
    auto binding = storage.add(&item);
    CHECK(storage.empty() == false);
    CHECK(storage.full() == (index == 2));
    CHECK(storage.size() == index + 1);
    CHECK(binding.button_index == index);
    CHECK(binding.event_group_index == (index > 1 ? 1 : 0));
    CHECK(binding.valid == true);
    index++;
  }

  CHECK(*storage[0] == 0);
  CHECK(*storage[1] == 1);
  CHECK(*storage[2] == 2);

  // Container is full, adding should fail
  uint32_t full_item = 12;
  auto binding = storage.add(&full_item);
  CHECK(storage.empty() == false);
  CHECK(storage.full() == true);
  CHECK(storage.size() == 3);
  CHECK(binding.button_index == 0);
  CHECK(binding.event_group_index == 0);
  CHECK(binding.valid == false);

  // Remove an item which doesn't exist
  auto removed = storage.remove(&full_item);
  CHECK(storage.full() == true);
  CHECK(storage.size() == 3);
  CHECK(removed == false);

  // Remove an item which does
  removed = storage.remove(&items[2]);
  CHECK(storage.full() == false);
  CHECK(storage.size() == 2);
  CHECK(removed == true);
}