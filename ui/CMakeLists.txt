# ui/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 在文件开始处设置 AUTOMOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 设置 AUTOUIC 搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms)

# 查找 Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)

# 手动列出源文件，排除图表相关文件
set(UI_SOURCES
    src/mainwindow.cpp
    src/adddevicedialog.cpp
    src/confirmdialog.cpp
    src/errordialog.cpp
    # 暂时注释掉图表相关文件
    # src/chart_3d_widget.cpp
    # src/chart_display_widget.cpp
    # src/data_visualization_widget.cpp
)

set(UI_HEADERS
    include/mainwindow.h
    include/adddevicedialog.h
    include/confirmdialog.h
    include/errordialog.h
    # 暂时注释掉图表相关文件
    # include/chart_3d_widget.h
    # include/chart_display_widget.h
    # include/data_visualization_widget.h
)

file(GLOB UI_FORMS "forms/*.ui")

# 创建库
add_library(ui_lib STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${UI_FORMS}
)

# 设置目标属性
set_target_properties(ui_lib PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/forms"
)

# 包含目录
target_include_directories(ui_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# 链接库
target_link_libraries(ui_lib PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    core_lib
    data_lib
    hardware_lib
    models_lib
    utils_lib
)