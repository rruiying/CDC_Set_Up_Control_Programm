# 硬件抽象层库
add_library(cdc_hardware STATIC
    # src/serial_interface.cpp
    # src/motor_interface.cpp
    # src/sensor_interface.cpp
)

target_link_libraries(cdc_hardware 
    PUBLIC 
    Qt6::Core 
    Qt6::SerialPort
    cdc_models
)

target_include_directories(cdc_hardware PUBLIC
    include
)