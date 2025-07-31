# UI库
add_library(cdc_ui STATIC
    src/mainwindow.cpp
    src/adddevicedialog.cpp
    # src/manualcalibrationdialog.cpp
)

target_link_libraries(cdc_ui 
    PUBLIC 
    Qt6::Widgets
    cdc_app
    cdc_core
    cdc_models
)

target_include_directories(cdc_ui PUBLIC
    include
)