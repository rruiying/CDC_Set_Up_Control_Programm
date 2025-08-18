# src/utils/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(UTILS_HEADERS
    include/config_manager.h
    include/logger.h
    include/math_utils.h
)

set(UTILS_SOURCES
    src/config_manager.cpp
    src/logger.cpp
    src/math_utils.cpp
)

add_library(utils_lib STATIC
    ${UTILS_SOURCES}
    ${UTILS_HEADERS}
)

target_include_directories(utils_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(utils_lib PUBLIC
    Qt6::Core
)

set_target_properties(utils_lib PROPERTIES AUTOMOC ON)
