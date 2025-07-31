# 数据处理库
add_library(cdc_data STATIC
    # src/data_processor.cpp
    # src/export_manager.cpp
    # src/file_manager.cpp
)

target_link_libraries(cdc_data 
    PUBLIC 
    Qt6::Core
    cdc_models
)

target_include_directories(cdc_data PUBLIC
    include
)