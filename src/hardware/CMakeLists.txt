# src/hardware/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(HARDWARE_HEADERS
    include/command_protocol.h
    include/sensor_interface.h
    include/serial_interface.h
)

set(HARDWARE_SOURCES
    src/command_protocol.cpp
    src/sensor_interface.cpp
    src/serial_interface.cpp
)

add_library(hardware_lib STATIC
    ${HARDWARE_SOURCES}
    ${HARDWARE_HEADERS}
)

target_include_directories(hardware_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hardware_lib PUBLIC
    Qt6::Core
    Qt6::SerialPort
    models_lib
    utils_lib
)

set_target_properties(hardware_lib PROPERTIES AUTOMOC ON)
