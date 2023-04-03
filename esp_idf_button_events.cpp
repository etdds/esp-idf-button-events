#include <esp_idf_button_events/esp_idf_button_events.hpp>

// #define BUTTON_GPIO_PIN GPIO_NUM_35

#define TAG                 "Button"
#define BUTTON_PRESS_EVENT  0x1
#define BUTTON_TIMER_EVENT  0x2
#define BUTTON_REPEAT_EVENT 0x4

// #include "esp_event_base.h"

// ESP_EVENT_DEFINE_BASE(BUTTON1);
// ESP_EVENT_DEFINE_BASE(BUTTON2);

// #define ESP_EVENT_DECLARE_BASE(id) extern esp_event_base_t id
// #define ESP_EVENT_DEFINE_BASE(id) esp_event_base_t id = #id

esp_event_base_t event_bases[] = {
  "BUTTON1",
  "BUTTON2",
};


constexpr State to_state(bool level) {
  return level ? State::NOT_PRESSED : State::PRESSED;
}

static void IRAM_ATTR button_isr_handler(void* arg) {
  auto b = static_cast<Button*>(arg);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if(!b->debounce) {
    xEventGroupSetBitsFromISR(b->_event_group, BUTTON_PRESS_EVENT << b->event_shift, &xHigherPriorityTaskWoken);
  }
  if(xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void timer_debounce_callback(void* arg) {
  auto b = static_cast<Button*>(arg);
  b->state = to_state(gpio_get_level(b->pin));
  xEventGroupSetBits(b->_event_group, BUTTON_TIMER_EVENT << b->event_shift);
}

void timer_press_callback(void* arg) {
  auto b = static_cast<Button*>(arg);
  esp_timer_start_once(b->press_timer, 500000);
  xEventGroupSetBits(b->_event_group, BUTTON_REPEAT_EVENT << b->event_shift);
}

EventManager& EventManager::instance() {
  static EventManager _instance;
  return _instance;
};

size_t EventManager::add_button(Button* button) {
  size_t index = 0;
  for(auto& b: buttons) {
    if(b == nullptr) {
      b = button;
      break;
    }
    index++;
  }
  _button_count++;
  return index * 3;
}

EventManager::EventManager() : _event_group{xEventGroupCreate()}, buttons{{nullptr}} {
  _button_count = 0;
  xTaskCreate(
    [](void* arg) {
      auto manager = static_cast<EventManager*>(arg);
      manager->task_loop();
    },
    "button_task", configMINIMAL_STACK_SIZE * 4, this, 5, NULL);

  esp_event_loop_args_t loop_with_task_args = {.queue_size = 5,
                                               .task_name = "loop_task",  // task will be created
                                               .task_priority = uxTaskPriorityGet(NULL),
                                               .task_stack_size = 3072,
                                               .task_core_id = tskNO_AFFINITY};

  ESP_ERROR_CHECK(esp_event_loop_create(&loop_with_task_args, &loop_with_task));
};

void EventManager::task_loop() {
  while(true) {
    EventBits_t bits = xEventGroupWaitBits(_event_group, 0xFFFFFF, pdTRUE, pdFALSE, portMAX_DELAY);
    while(bits) {
      size_t shift = 0;
      for(auto i = 0; i < _button_count; i++) {
        if(bits & (0x7 << i * 3)) {
          shift = i;
          break;
        }
      }
      auto button = buttons[shift];
      bits = bits >> shift * 3;
      if(bits & BUTTON_PRESS_EVENT) {
        if(!button->debounce) {
          button->debounce = true;
          esp_timer_start_once(button->debounce_timer, 50000);
        }
      }

      if(bits & BUTTON_TIMER_EVENT) {
        button->debounce = false;
        if(button->state == State::PRESSED) {
          button->down_time = esp_timer_get_time();
          esp_timer_start_once(button->press_timer, 3000000);
          esp_event_post_to(loop_with_task, event_bases[shift], BUTTON_DOWN, NULL, 0, portMAX_DELAY);
        }
        else {
          auto current_time = esp_timer_get_time();
          auto delta_ms = (current_time - button->down_time) / 1000;
          esp_timer_stop(button->press_timer);
          esp_event_post_to(loop_with_task, event_bases[shift], BUTTON_UP, NULL, 0, portMAX_DELAY);
          if(delta_ms > 3000) {
            esp_event_post_to(loop_with_task, event_bases[shift], BUTTON_LONG_PRESS, NULL, 0, portMAX_DELAY);
          }
          else if(delta_ms > 100) {
            esp_event_post_to(loop_with_task, event_bases[shift], BUTTON_PRESS, NULL, 0, portMAX_DELAY);
          }
          else {
            // No press
          }
        }
      }
      if(bits & BUTTON_REPEAT_EVENT) {
        esp_event_post_to(loop_with_task, event_bases[shift], BUTTON_HELD, NULL, 0, portMAX_DELAY);
      }
      bits &= ~(0x7);
    }
  }
}


Button::Button(gpio_num_t pin) :
  pin(pin), debounce{false}, inverted{false}, event_shift(0), state{State::NOT_PRESSED}, down_time(0), _manager(EventManager::instance()) {
  gpio_config_t io_conf = {
    .pin_bit_mask = 1ULL << pin,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_ANYEDGE,
  };
  gpio_config(&io_conf);

  _event_group = _manager._event_group;

  esp_timer_create_args_t timer_param = {.callback = timer_debounce_callback,
                                         .arg = this,
                                         .dispatch_method = ESP_TIMER_TASK,
                                         .name = "db_timer",
                                         .skip_unhandled_events = false};


  esp_timer_create(&timer_param, &debounce_timer);

  timer_param.callback = timer_press_callback;
  timer_param.name = "press_timer";

  esp_timer_create(&timer_param, &press_timer);

  static bool i = false;
  if(!i) {
    gpio_install_isr_service(0);
    i = true;
  }
  gpio_isr_handler_add(pin, button_isr_handler, this);
  event_shift = _manager.add_button(this);
  event_base = event_bases[event_shift / 3];
}

void Button::add_handler(esp_event_handler_t handler, void* arg, int32_t event) {
  esp_event_handler_instance_register_with(_manager.loop_with_task, event_base, event, handler, arg, nullptr);
}