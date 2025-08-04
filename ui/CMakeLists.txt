# ui/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 查找Qt5组件
find_package(Qt5 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Gui 
    3DCore 
    3DRender 
    3DInput 
    3DExtras
)

# UI文件
set(UI_FORMS
    forms/mainwindow.ui
    forms/adddevicedialog.ui
    forms/confirmdialog.ui
    forms/errordialog.ui
)

# 头文件
set(UI_HEADERS
    include/mainwindow.h
    include/adddevicedialog.h
    include/confirmdialog.h
    include/errordialog.h
    include/data_visualization_widget.h    # 第五页主控件
    include/chart_display_widget.h         # 2D图表控件
    include/chart_3d_widget.h             # 3D图表控件
)

# 源文件
set(UI_SOURCES
    src/mainwindow.cpp
    src/adddevicedialog.cpp
    src/confirmdialog.cpp
    src/errordialog.cpp
    src/data_visualization_widget.cpp      # 第五页主控件
    src/chart_display_widget.cpp          # 2D图表控件
    src/chart_3d_widget.cpp               # 3D图表控件
)

# 自动生成的UI头文件
qt5_wrap_ui(UI_HEADERS_GENERATED ${UI_FORMS})

# 创建UI库
add_library(ui_lib STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${UI_HEADERS_GENERATED}
)

# 设置包含目录
target_include_directories(ui_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}  # 用于包含自动生成的ui_*.h文件
    ${CMAKE_SOURCE_DIR}/third_party/qcustomplot  # QCustomPlot头文件
)

# 链接Qt库和其他模块
target_link_libraries(ui_lib PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::3DCore      # Qt3D核心
    Qt5::3DRender    # Qt3D渲染
    Qt5::3DInput     # Qt3D输入
    Qt5::3DExtras    # Qt3D扩展
    qcustomplot      # QCustomPlot库
    core_lib
    data_lib
    hardware_lib
    models_lib
    utils_lib
)

# 设置自动MOC
set_target_properties(ui_lib PROPERTIES AUTOMOC ON)

# 定义USE_QT3D宏
target_compile_definitions(ui_lib PUBLIC USE_QT3D)