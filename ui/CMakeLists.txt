# ui/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 在文件开始处设置 AUTOMOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 查找 Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)

# 收集所有源文件
file(GLOB UI_SOURCES "src/*.cpp")
file(GLOB UI_HEADERS "include/*.h")
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
)

# 包含目录
target_include_directories(ui_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
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