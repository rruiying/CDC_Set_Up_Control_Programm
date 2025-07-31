cmake_minimum_required(VERSION 3.16)
project(CDC_Set_Up_Control_Program VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets SerialPort)
find_package(GTest REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(ui)
add_subdirectory(tests)

# Main executable
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    app_lib
    ui_lib
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)