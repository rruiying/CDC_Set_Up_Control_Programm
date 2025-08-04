cmake_minimum_required(VERSION 3.16)

add_library(qcustomplot STATIC
    qcustomplot.cpp
    qcustomplot.h
)

target_link_libraries(qcustomplot
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::PrintSupport
)

target_include_directories(qcustomplot PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(qcustomplot PROPERTIES 
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)