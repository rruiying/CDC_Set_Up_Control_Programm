set(UI_SOURCES
    src/mainwindow.cpp
    src/adddevicedialog.cpp
    src/manualcalibrationdialog.cpp
)

set(UI_HEADERS
    include/mainwindow.h
    include/adddevicedialog.h
    include/manualcalibrationdialog.h
)

set(UI_FORMS
    forms/mainwindow.ui
    forms/adddevicedialog.ui
    forms/manualcalibrationdialog.ui
)

add_library(ui_lib ${UI_SOURCES} ${UI_HEADERS} ${UI_FORMS})
target_include_directories(ui_lib PUBLIC include)
target_link_libraries(ui_lib
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialPort
    core_lib
    hardware_lib
    models_lib
    utils_lib
)