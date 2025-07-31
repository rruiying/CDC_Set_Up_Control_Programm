cmake_minimum_required(VERSION 3.21)
project(CDC_Set_Up_Control_Programm VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6设置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

# 启用测试
enable_testing()

# 全局包含目录
include_directories(
    src/models/include
    src/core/include
    src/hardware/include
    src/data/include
    src/utils/include
    src/app/include
    ui/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

# 添加子目录
add_subdirectory(src)
add_subdirectory(ui)
add_subdirectory(tests)

# 手动处理UI文件
set(UI_FILES
    ui/forms/mainwindow.ui
    ui/forms/adddevicedialog.ui
    ui/forms/manualcalibrationdialog.ui
)

qt6_wrap_ui(UI_HEADERS ${UI_FILES})

# 主可执行文件
qt6_add_executable(CDC_Set_Up_Control_Programm
    src/main.cpp
    ${UI_HEADERS}
)

# 链接所有库
target_link_libraries(CDC_Set_Up_Control_Programm 
    PRIVATE 
    Qt6::Widgets 
    Qt6::SerialPort
    cdc_models
    cdc_utils
    cdc_hardware
    cdc_data
    cdc_core
    cdc_app
    cdc_ui
)