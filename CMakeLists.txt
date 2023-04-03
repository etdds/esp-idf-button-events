set(COMPONET_SRC 
    "esp_idf_button_events.cpp"
)

idf_component_register(
    SRCS 
        ${COMPONET_SRC}
    REQUIRES 
        "esp_timer"
        "esp_event"
        "driver"
    INCLUDE_DIRS 
        "include"
)
