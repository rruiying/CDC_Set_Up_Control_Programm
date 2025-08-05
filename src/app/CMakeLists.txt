# src/app/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(APP_HEADERS
    include/application.h
)

set(APP_SOURCES
    src/application.cpp
)

add_library(app_lib STATIC
    ${APP_SOURCES}
    ${APP_HEADERS}
)

target_include_directories(app_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src  # 添加源码根目录
)

target_link_libraries(app_lib PUBLIC
    Qt5::Core
    Qt5::Widgets
    core_lib
    data_lib
    hardware_lib
    models_lib
    ui_lib
    utils_lib
)

set_target_properties(app_lib PROPERTIES AUTOMOC ON)