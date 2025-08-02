# src/utils/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 创建utils库
add_library(utils STATIC
    src/logger.cpp
    src/config_manager.cpp
    src/math_utils.cpp
)

target_include_directories(utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 设置C++标准
target_compile_features(utils PUBLIC cxx_std_17)

# 链接需要的库
if(WIN32)
    target_link_libraries(utils PUBLIC
        # Windows需要的库
    )
elseif(UNIX)
    target_link_libraries(utils PUBLIC
        pthread  # 多线程支持
    )
endif()