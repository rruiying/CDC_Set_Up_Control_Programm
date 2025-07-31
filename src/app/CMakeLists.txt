set(APP_SOURCES
    src/application.cpp
)

set(APP_HEADERS
    include/application.h
)

add_library(app_lib ${APP_SOURCES} ${APP_HEADERS})
target_include_directories(app_lib PUBLIC include)
target_link_libraries(app_lib
    Qt6::Core
    Qt6::Widgets
    core_lib
    hardware_lib
)