# 工具库
add_library(cdc_utils STATIC
    # src/math_utils.cpp
    # src/config_manager.cpp
    # src/logger.cpp
)

target_link_libraries(cdc_utils PUBLIC Qt6::Core)

target_include_directories(cdc_utils PUBLIC
    include
)