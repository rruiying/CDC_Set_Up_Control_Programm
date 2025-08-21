set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui PrintSupport SerialPort)

set(UI_SOURCES
    src/mainwindow.cpp
    src/adddevicedialog.cpp
    src/confirmdialog.cpp
    src/errordialog.cpp
)

set(UI_HEADERS
    include/mainwindow.h
    include/adddevicedialog.h
    include/confirmdialog.h
    include/errordialog.h
)

set(UI_FORMS
    forms/mainwindow.ui
    forms/adddevicedialog.ui
)

add_library(ui_lib STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${UI_FORMS}
)

target_include_directories(ui_lib 
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ui_lib
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        Qt6::PrintSupport
        Qt6::SerialPort
        controller_lib
    PRIVATE
        core_lib
        data_lib
        hardware_lib
        models_lib
        utils_lib
)

set_target_properties(ui_lib PROPERTIES AUTOMOC ON AUTOUIC ON)