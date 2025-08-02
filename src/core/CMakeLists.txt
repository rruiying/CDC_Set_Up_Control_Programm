# src/core/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 创建core库
add_library(core STATIC
    src/sensor_manager.cpp
    src/motor_controller.cpp
    src/safety_manager.cpp
    src/data_recorder.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 设置C++标准
target_compile_features(core PUBLIC cxx_std_17)

# 链接依赖的库
target_link_libraries(core PUBLIC
    hardware
    models
    utils
)

# 如果是Windows平台，定义M_PI
if(WIN32)
    target_compile_definitions(core PRIVATE _USE_MATH_DEFINES)
endif()