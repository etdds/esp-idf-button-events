if(NOT DEFINED ESP_PLATFORM)
    cmake_minimum_required(VERSION 3.0.0)
    project(esp-idf-button-events)
endif()

set(COMPONENT_NAME "esp-idf-button-events")

set(COMPONENT_SRCS 
    "esp_idf_button_events.cpp"
)

set(COMPONENT_REQUIRES 
    "esp_timer"
    "esp_event"
    "driver"
)

set(COMPONENT_ADD_INCLUDEDIRS 
    "include"
)

set(COMPONET_SRC 
    "esp_idf_button_events.cpp"
)

if(DEFINED ESP_PLATFORM)

idf_component_register(
    SRCS 
        ${COMPONENT_SRCS}
    REQUIRES 
        ${COMPONENT_REQUIRES}
    INCLUDE_DIRS 
        ${COMPONENT_ADD_INCLUDEDIRS}
)
else()

enable_testing()
    # Add a test executable for each test file found in tests
    file(GLOB TEST_SRCS ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp)
    foreach(test_src ${TEST_SRCS})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
        target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/doctest)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

endif()