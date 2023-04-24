# ESP IDF Button Event Library

A ESP IDF component for managing buttons tied to GPIO pins in a publish / suscribe fashion. The component internally handles button debouncing and allows subscription to the following events:
 - Button down event.
 - Button up event.
 - Button press event.
 - Button long press event.
 - Button held event.
 - Button held repeat event.

Generated events can be a one to many subscribers, or multiple events to a single subscriber. The component is managed by ISR events and timer callbacks. Minimal to no processing time is required if no buttons are being toggled.

# Example Usage

The following example is located at `examples/buttons`. Once cloned it can be build and run with:

```bash

# Configure for ttgo-tdisplay. This only effects the GPIO numbers used in the board. Other boards can be tested by changing
# The GPIO numbers in the example
cd examples/buttons && rm -f sdkconfig* && cp ttgo-tdisplay.defaults sdkconfig.defaults && idf.py set-target esp32

# Build the example (from the examples/buttons directory)
idf.py build

# Flash
idf.py -p (PORT) flash

# Monitor output
idf.py -p (PORT) monitor

# Clean
idf.py fullclean

```

The source for the example:

```c++
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include <esp_idf_button_events/button.hpp>

#include "esp_system.h"

#define LOG_TAG "MAIN"

// Specify namespace to reduce verbosity.
using namespace ButtonEvents;

// Specify a dummy class to demonstrate calling a class member on event handler.
class Object {
 public:
  void handler() { ESP_LOGI(LOG_TAG, "Class handler called"); }
};

// Three event handlers which are called on button events.
static void press_any(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  auto event = EventData(event_data);
  if(handler_args) {
    auto arg = static_cast<int*>(handler_args);
  }
  ESP_LOGI(LOG_TAG, "Any handler: %s: ID %li, arg: %d", event.button->name(), id, *arg);
}

static void press_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  auto event = EventData(event_data);
  ESP_LOGI(LOG_TAG, "Press handler: %s: ID %li", event.button->name(), id);
}

static void long_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
  auto event = EventData(event_data);
  ESP_LOGI(LOG_TAG, "Long handler: %s: ID %li", event.button->name(), id);
}

extern "C" void app_main(void) {
  // Create a button using the Kconfig default options.
  Button* button_a = Button::create("Button A", GPIO_NUM_0);

  // Create another button, with custom button specific configuration.
  Button* button_b = Button::create("Button B", GPIO_NUM_35)
                       .inverted(false)
                       .pull_up(true)
                       .pull_down(false)
                       .debounce_ms(50)
                       .short_press_ms(100)
                       .long_press_ms(1000)
                       .hold_press_ms(3000)
                       .hold_repeat_ms(200);


  // Example value to pass through to the event handler.
  uint32_t handler_value = 55;

  // An object who's method is called on an event through a lambda.
  Object object;

  // Add handler for all events on button B.
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_DOWN);
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_UP);
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_PRESS);
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_LONG_PRESS);
  button_b->add_handler(press_any, &handler_value, EventType::BUTTON_HELD);

  // Add handler specifically press and long events on button B
  button_b->add_handler(press_handler, &handler_value, EventType::BUTTON_PRESS);
  button_b->add_handler(long_handler, &handler_value, EventType::BUTTON_LONG_PRESS);

  // Add press and long handler for button A.
  button_a->add_handler(long_handler, &handler_value, EventType::BUTTON_LONG_PRESS);
  button_a->add_handler(press_handler, &handler_value, EventType::BUTTON_PRESS);

  button_a->add_handler(
    [](void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
      auto o = static_cast<Object*>(handler_args);
      o->handler();
    },
    &object, EventType::BUTTON_PRESS);

  ESP_LOGI(LOG_TAG, "Waiting for events...");
  while(1) {
    // Delay the task for 1000ms (1 second)
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

```

Possible output:
```
I (0) cpu_start: App cpu up.
I (210) cpu_start: Pro cpu start user code
I (210) cpu_start: cpu freq: 160000000 Hz
I (210) cpu_start: Application information:
I (215) cpu_start: Project name:     buttons
I (220) cpu_start: App version:      1
I (224) cpu_start: Compile time:     Apr 24 2023 09:06:09
I (230) cpu_start: ELF file SHA256:  c431627ce8601034...
I (236) cpu_start: ESP-IDF:          v5.0.1
I (241) cpu_start: Min chip rev:     v0.0
I (246) cpu_start: Max chip rev:     v3.99 
I (251) cpu_start: Chip rev:         v3.0
I (256) heap_init: Initializing. RAM available for dynamic allocation:
I (263) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (269) heap_init: At 3FFB2988 len 0002D678 (181 KiB): DRAM
I (275) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (281) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (288) heap_init: At 4008CB50 len 000134B0 (77 KiB): IRAM
I (296) spi_flash: detected chip: winbond
I (299) spi_flash: flash io: dio
W (303) spi_flash: Detected size(16384k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (317) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (328) gpio: GPIO[0]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3 
I (338) gpio: GPIO[35]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3 
I (348) MAIN: Waiting for events...
I (2158) MAIN: Press handler: Button A: ID 2
I (2158) MAIN: Class handler called
I (3538) MAIN: Any handler: Button B: ID 1, arg: 55
I (3718) MAIN: Any handler: Button B: ID 0, arg: 55
I (3718) MAIN: Any handler: Button B: ID 2, arg: 55
```

# Installation

## ESP IDF component manager

Add to your main `idf_component.yml` file, under dependencies.

```yaml
dependencies:
  esp-idf-button-events:
    git: https://github.com/etdds/esp-idf-button-events.git
```

## Direct component
Add the repository as a standard IDF component.
```
git clone https://github.com/etdds/esp-idf-button-events.git components/esp-idf-button-events
```

## Submodule component
Add the repository as a standard IDF component.
```
git submodule add https://github.com/etdds/esp-idf-button-events.git components/esp-idf-button-events
```

# Configuration

Default configuration for buttons and tasks is done using KConfig. When using the ESP-IDF component manager, use `idf.py menuconfig` and browse to Component config -> ESP IDF Button Events

# Limitiations / TODO

Some known limitations which may be addressed in the future. Feel free to implement and open a pull request, or open an issue to disccuss.
  - There is a hard limit on the number of buttons which can be used (6). Some modification to the used FreeRTOS event bits needs to be done to support more.
  - Buttons are never deinitialised, nor is there a method to release the memory allocated for the button. This should be added together.
  - The button could be fetched by name, since the event manager keeps track of the button handlers. That way, the button handler doesn't need to be tracked externally.
  - Timers for individual buttons have the same name.
  - Could add an option to reduce the number of possible event. E.g filter out press down and press up events. Reducing load.
  - Could add the option to use the inbuilt ESP event loop, to save resources.
  - The manager task and event loop stack size / prioriy are just initial values. Some profiling could be done to choose better defaults.