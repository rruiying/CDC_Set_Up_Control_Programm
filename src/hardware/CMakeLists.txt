# src/hardware/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 创建hardware库
add_library(hardware STATIC
    src/command_protocol.cpp
    src/serial_interface.cpp
    src/motor_interface.cpp
    src/sensor_interface.cpp
)

target_include_directories(hardware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 设置C++标准
target_compile_features(hardware PUBLIC cxx_std_17)

# 链接依赖的库
target_link_libraries(hardware PUBLIC
    models
    utils
)

# 平台相关的库
if(WIN32)
    target_link_libraries(hardware PUBLIC
        setupapi
    )
elseif(UNIX)
    target_link_libraries(hardware PUBLIC
        pthread
    )
endif()

# 如果是Windows平台，定义M_PI
if(WIN32)
    target_compile_definitions(hardware PRIVATE _USE_MATH_DEFINES)
endif()