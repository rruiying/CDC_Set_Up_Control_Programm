# 核心业务逻辑库
add_library(cdc_core STATIC
    # src/motor_controller.cpp
    # src/sensor_manager.cpp
    # src/safety_manager.cpp
    # src/calibration_engine.cpp
)

target_link_libraries(cdc_core 
    PUBLIC 
    Qt6::Core
    cdc_models
    cdc_hardware
    cdc_data
    cdc_utils
)

target_include_directories(cdc_core PUBLIC
    include
)