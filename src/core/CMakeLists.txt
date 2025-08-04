# src/core/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(CORE_HEADERS
    include/data_recorder.h
    include/motor_controller.h
    include/safety_manager.h
    include/sensor_manager.h
)

set(CORE_SOURCES
    src/data_recorder.cpp
    src/motor_controller.cpp
    src/safety_manager.cpp
    src/sensor_manager.cpp
)

add_library(core_lib STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

target_include_directories(core_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(core_lib PUBLIC
    Qt5::Core
    models_lib
    hardware_lib
    utils_lib
)

set_target_properties(core_lib PROPERTIES AUTOMOC ON)