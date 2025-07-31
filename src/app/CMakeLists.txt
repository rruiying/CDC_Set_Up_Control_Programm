# 应用层库
add_library(cdc_app STATIC
    # src/application.cpp
)

target_link_libraries(cdc_app 
    PUBLIC 
    Qt6::Core
    cdc_core
    cdc_models
)

target_include_directories(cdc_app PUBLIC
    include
)