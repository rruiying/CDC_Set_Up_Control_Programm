# src/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 添加各个模块的子目录
add_subdirectory(app)
add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(hardware)
add_subdirectory(models)
add_subdirectory(utils)