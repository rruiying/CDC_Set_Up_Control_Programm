# src/data/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 头文件
set(DATA_HEADERS
    include/data_processor.h
    include/export_manager.h
    include/file_manager.h
    include/csv_analyzer.h      # 新增：CSV分析器
)

# 源文件
set(DATA_SOURCES
    src/data_processor.cpp
    src/export_manager.cpp
    src/file_manager.cpp
    src/csv_analyzer.cpp        # 新增：CSV分析器
)

# 创建数据处理库
add_library(data_lib STATIC
    ${DATA_SOURCES}
    ${DATA_HEADERS}
)

# 设置包含目录
target_include_directories(data_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接依赖库
target_link_libraries(data_lib PUBLIC
    Qt6::Core
    models_lib
    utils_lib
)

# 设置自动MOC（如果需要）
set_target_properties(data_lib PROPERTIES AUTOMOC ON)
