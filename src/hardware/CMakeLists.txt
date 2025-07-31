set(HARDWARE_SOURCES
    src/serial_interface.cpp
    src/motor_interface.cpp
    src/sensor_interface.cpp
)

set(HARDWARE_HEADERS
    include/serial_interface.h
    include/motor_interface.h
    include/sensor_interface.h
)

add_library(hardware_lib ${HARDWARE_SOURCES} ${HARDWARE_HEADERS})
target_include_directories(hardware_lib PUBLIC include)
target_link_libraries(hardware_lib
    Qt6::Core
    Qt6::SerialPort
    utils_lib
)