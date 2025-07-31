set(CORE_SOURCES
    src/motor_controller.cpp
    src/sensor_manager.cpp
    src/calibration_engine.cpp
    src/safety_manager.cpp
    src/data_recorder.cpp
)

set(CORE_HEADERS
    include/motor_controller.h
    include/sensor_manager.h
    include/calibration_engine.h
    include/safety_manager.h
    include/data_recorder.h
)

add_library(core_lib ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(core_lib PUBLIC include)
target_link_libraries(core_lib
    Qt6::Core
    hardware_lib
    models_lib
    utils_lib
)