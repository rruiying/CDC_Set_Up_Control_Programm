# 根目录 CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(CDC_Control_System VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置自动MOC、UIC、RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找Qt5组件
find_package(Qt5 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Gui 
    SerialPort 
    Charts 
    PrintSupport    # QCustomPlot需要
    3DCore          # Qt3D核心
    3DRender        # Qt3D渲染
    3DInput         # Qt3D输入
    3DExtras        # Qt3D扩展
)

# 设置Qt5目录（如果需要）
# set(CMAKE_PREFIX_PATH "C:/Qt/5.15.2/msvc2019_64")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# 启用测试（可选）
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 添加子目录
add_subdirectory(third_party)  # 第三方库
add_subdirectory(src)           # 源代码
add_subdirectory(ui)            # UI代码

# 主可执行文件
add_executable(CDC_Control_System
    src/main.cpp
)

# 链接库
target_link_libraries(CDC_Control_System
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    ui_lib
    core_lib
    data_lib
    hardware_lib
    models_lib
    utils_lib
    qcustomplot     # QCustomPlot库
)

# Windows特定设置
if(WIN32)
    set_target_properties(CDC_Control_System PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # 部署Qt库（仅在Release模式）
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET CDC_Control_System POST_BUILD
            COMMAND ${CMAKE_PREFIX_PATH}/bin/windeployqt.exe $<TARGET_FILE:CDC_Control_System>
        )
    endif()
endif()

# 安装规则
install(TARGETS CDC_Control_System
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装配置文件和资源
install(DIRECTORY runtime/ DESTINATION bin)
install(FILES README.md DESTINATION .)