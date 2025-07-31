# 查找Google Test
find_package(GTest REQUIRED)

# SensorData测试
add_executable(test_sensor_data
    test_main.cpp
    data_tests/test_sensor_data.cpp
)

target_link_libraries(test_sensor_data
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
    cdc_models
)

# 注册到CTest
include(GoogleTest)
gtest_discover_tests(test_sensor_data)

# 添加测试
add_test(NAME SensorDataTests COMMAND test_sensor_data)