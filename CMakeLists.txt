cmake_minimum_required(VERSION 3.16)
project(CDC_Control_System VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(BUILD_GUI "Build with Qt GUI" ON)
option(BUILD_TESTS "Build test programs" OFF)

if(BUILD_GUI)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui PrintSupport SerialPort)
endif()

add_subdirectory(src)

if(BUILD_GUI)
    add_subdirectory(ui) 
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_GUI)
    add_executable(CDC_Control_System src/main.cpp)

    target_include_directories(CDC_Control_System
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/ui/include
    )

    target_link_libraries(CDC_Control_System
        PRIVATE
            app_lib
            ui_lib
            controller_lib
            hardware_lib
            data_lib
            models_lib
            core_lib
            utils_lib
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
    )
    
    if(WIN32)
        set_target_properties(CDC_Control_System PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
else()
    # 命令行版本
    add_executable(CDC_Control_System_CLI src/main_cli.cpp)
    target_link_libraries(CDC_Control_System_CLI
        PRIVATE
            controller_lib
            hardware_lib
            data_lib
    )
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)