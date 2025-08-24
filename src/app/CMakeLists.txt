add_library(controller_lib STATIC
    include/application_controller.h
    src/application_controller.cpp
)

target_include_directories(controller_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(controller_lib
    PUBLIC
        core_lib
        data_lib
        hardware_lib
        models_lib
        utils_lib
)

add_library(app_lib STATIC
    include/application.h
    src/application.cpp
)

target_include_directories(app_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(app_lib
    PUBLIC
        controller_lib
    PRIVATE 
        Qt6::Core
        Qt6::Widgets
        ui_lib
)

set_target_properties(app_lib PROPERTIES AUTOMOC ON)
