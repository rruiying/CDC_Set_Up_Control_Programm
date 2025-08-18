# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 查找GTest（如果使用Google Test）
find_package(GTest)

if(NOT GTest_FOUND)
    # 如果系统没有安装GTest，可以使用FetchContent下载
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
endif()

# 启用测试
enable_testing()

# 测试源文件
set(TEST_SOURCES
    test_main.cpp
    # Core tests
    core_tests/test_data_recorder.cpp
    core_tests/test_motor_controller.cpp
    core_tests/test_safety_manager.cpp
    core_tests/test_sensor_manager.cpp
    # Data tests
    data_tests/test_data_processor.cpp
    data_tests/test_export_manager.cpp
    data_tests/test_file_manager.cpp
    data_tests/test_csv_analyzer.cpp      # 新增
    # Hardware tests
    hardware_tests/test_command_protocol.cpp
    hardware_tests/test_motor_interface.cpp
    hardware_tests/test_sensor_interface.cpp
    hardware_tests/test_serial_interface.cpp
    # Models tests
    models_tests/test_device_info.cpp
    models_tests/test_measurement_data.cpp
    models_tests/test_sensor_data.cpp
    models_tests/test_system_config.cpp
    # Utils tests
    utils_tests/test_config_manager.cpp
    utils_tests/test_logger.cpp
    utils_tests/test_math_utils.cpp
    # UI tests（新增）
    ui_tests/test_data_visualization.cpp
)

# 创建测试可执行文件
add_executable(CDC_Tests ${TEST_SOURCES})

# 链接库
target_link_libraries(CDC_Tests
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
    Qt6::Widgets
    Qt6::Test       # Qt测试框架
    core_lib
    data_lib
    hardware_lib
    models_lib
    utils_lib
    ui_lib
    qcustomplot
)

# 添加测试
add_test(NAME CDC_Tests COMMAND CDC_Tests)

# 设置测试的工作目录
set_tests_properties(CDC_Tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
