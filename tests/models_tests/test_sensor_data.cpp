// tests/models_tests/test_sensor_data.cpp
#include <gtest/gtest.h>
#include "models/include/sensor_data.h"
#include <cmath>

class SensorDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前的初始化
    }
    
    void TearDown() override {
        // 测试后的清理
    }
};

// 测试默认构造函数
TEST_F(SensorDataTest, DefaultConstructor) {
    SensorData data;
    
    // 验证所有值都被初始化为0
    EXPECT_DOUBLE_EQ(data.getUpperSensor1(), 0.0);
    EXPECT_DOUBLE_EQ(data.getUpperSensor2(), 0.0);
    EXPECT_DOUBLE_EQ(data.getLowerSensor1(), 0.0);
    EXPECT_DOUBLE_EQ(data.getLowerSensor2(), 0.0);
    EXPECT_DOUBLE_EQ(data.getTemperature(), 0.0);
    EXPECT_DOUBLE_EQ(data.getMeasuredAngle(), 0.0);
    EXPECT_DOUBLE_EQ(data.getMeasuredCapacitance(), 0.0);
    EXPECT_FALSE(data.isValid());
}

// 测试带参数的构造函数
TEST_F(SensorDataTest, ParameterizedConstructor) {
    SensorData data(12.5, 13.0, 156.2, 156.8, 23.5, 2.5, 157.3);
    
    EXPECT_DOUBLE_EQ(data.getUpperSensor1(), 12.5);
    EXPECT_DOUBLE_EQ(data.getUpperSensor2(), 13.0);
    EXPECT_DOUBLE_EQ(data.getLowerSensor1(), 156.2);
    EXPECT_DOUBLE_EQ(data.getLowerSensor2(), 156.8);
    EXPECT_DOUBLE_EQ(data.getTemperature(), 23.5);
    EXPECT_DOUBLE_EQ(data.getMeasuredAngle(), 2.5);
    EXPECT_DOUBLE_EQ(data.getMeasuredCapacitance(), 157.3);
    EXPECT_TRUE(data.isValid());
}

// 测试平均高度计算
TEST_F(SensorDataTest, CalculateAverageHeight) {
    SensorData data(10.0, 14.0, 150.0, 150.0, 20.0, 0.0, 100.0);
    
    // 平均高度 = (10.0 + 14.0) / 2 = 12.0
    EXPECT_DOUBLE_EQ(data.getAverageHeight(), 12.0);
}

// 测试角度偏差计算
TEST_F(SensorDataTest, CalculateAngleOffset) {
    SensorData data(10.0, 15.0, 150.0, 150.0, 20.0, 0.0, 100.0);
    
    // 假设传感器间距为100mm（需要根据实际调整）
    // angle = atan((15.0 - 10.0) / 100.0) * 180 / PI
    double expectedAngle = std::atan((15.0 - 10.0) / 100.0) * 180.0 / M_PI;
    EXPECT_NEAR(data.getCalculatedAngle(), expectedAngle, 0.01);
}

// 测试平均地面距离
TEST_F(SensorDataTest, CalculateAverageGroundDistance) {
    SensorData data(10.0, 10.0, 155.0, 157.0, 20.0, 0.0, 100.0);
    
    // 平均地面距离 = (155.0 + 157.0) / 2 = 156.0
    EXPECT_DOUBLE_EQ(data.getAverageGroundDistance(), 156.0);
}

// 测试计算的上方距离
TEST_F(SensorDataTest, CalculateUpperDistance) {
    SensorData data(10.0, 10.0, 150.0, 150.0, 20.0, 0.0, 100.0);
    
    // 需要设置系统总高度和中间板高度
    data.setSystemHeight(200.0);
    data.setMiddlePlateHeight(25.0);
    
    // 计算的上方距离 = 总高度 - 地面距离 - 中间板高度 - 平均高度
    // = 200.0 - 150.0 - 25.0 - 10.0 = 15.0
    EXPECT_DOUBLE_EQ(data.getCalculatedUpperDistance(), 15.0);
}

// 测试数据有效性检查
TEST_F(SensorDataTest, DataValidation) {
    SensorData data;
    
    // 初始状态应该无效
    EXPECT_FALSE(data.isValid());
    
    // 设置一些合理的值
    data.setUpperSensors(10.0, 10.0);
    data.setLowerSensors(150.0, 150.0);
    data.setTemperature(25.0);
    data.setMeasuredAngle(0.0);
    data.setMeasuredCapacitance(100.0);
    
    EXPECT_TRUE(data.isValid());
    
    // 设置一个异常值（负距离）
    data.setUpperSensors(-10.0, 10.0);
    EXPECT_FALSE(data.isValid());
}

// 测试数据复制
TEST_F(SensorDataTest, CopyConstructor) {
    SensorData original(12.5, 13.0, 156.2, 156.8, 23.5, 2.5, 157.3);
    SensorData copy(original);
    
    EXPECT_DOUBLE_EQ(copy.getUpperSensor1(), original.getUpperSensor1());
    EXPECT_DOUBLE_EQ(copy.getUpperSensor2(), original.getUpperSensor2());
    EXPECT_DOUBLE_EQ(copy.getLowerSensor1(), original.getLowerSensor1());
    EXPECT_DOUBLE_EQ(copy.getLowerSensor2(), original.getLowerSensor2());
    EXPECT_DOUBLE_EQ(copy.getTemperature(), original.getTemperature());
    EXPECT_DOUBLE_EQ(copy.getMeasuredAngle(), original.getMeasuredAngle());
    EXPECT_DOUBLE_EQ(copy.getMeasuredCapacitance(), original.getMeasuredCapacitance());
}

// 测试从字符串解析（用于串口通信）
TEST_F(SensorDataTest, ParseFromString) {
    // 假设MCU返回格式："SENSORS:12.5,13.0,156.2,156.8,23.5,2.5,157.3"
    std::string dataString = "12.5,13.0,156.2,156.8,23.5,2.5,157.3";
    
    SensorData data;
    EXPECT_TRUE(data.parseFromString(dataString));
    
    EXPECT_DOUBLE_EQ(data.getUpperSensor1(), 12.5);
    EXPECT_DOUBLE_EQ(data.getUpperSensor2(), 13.0);
    EXPECT_DOUBLE_EQ(data.getLowerSensor1(), 156.2);
    EXPECT_DOUBLE_EQ(data.getLowerSensor2(), 156.8);
    EXPECT_DOUBLE_EQ(data.getTemperature(), 23.5);
    EXPECT_DOUBLE_EQ(data.getMeasuredAngle(), 2.5);
    EXPECT_DOUBLE_EQ(data.getMeasuredCapacitance(), 157.3);
}

// 测试错误格式解析
TEST_F(SensorDataTest, ParseFromInvalidString) {
    SensorData data;
    
    // 错误格式的数据
    EXPECT_FALSE(data.parseFromString("invalid,data"));
    EXPECT_FALSE(data.parseFromString("12.5,13.0"));  // 数据不完整
    EXPECT_FALSE(data.parseFromString(""));           // 空字符串
}

// 测试数据转换为字符串（用于日志记录）
TEST_F(SensorDataTest, ConvertToString) {
    SensorData data(12.5, 13.0, 156.2, 156.8, 23.5, 2.5, 157.3);
    
    std::string result = data.toString();
    
    // 验证字符串包含所有必要的数据
    EXPECT_TRUE(result.find("12.5") != std::string::npos);
    EXPECT_TRUE(result.find("13.0") != std::string::npos);
    EXPECT_TRUE(result.find("156.2") != std::string::npos);
    EXPECT_TRUE(result.find("156.8") != std::string::npos);
    EXPECT_TRUE(result.find("23.5") != std::string::npos);
    EXPECT_TRUE(result.find("2.5") != std::string::npos);
    EXPECT_TRUE(result.find("157.3") != std::string::npos);
}

// 测试数据范围检查
TEST_F(SensorDataTest, RangeValidation) {
    SensorData data;
    
    // 测试上传感器范围（0-100mm）
    EXPECT_TRUE(data.setUpperSensors(50.0, 50.0));
    EXPECT_FALSE(data.setUpperSensors(150.0, 50.0));  // 超出范围
    EXPECT_FALSE(data.setUpperSensors(-5.0, 50.0));   // 负值
    
    // 测试下传感器范围（0-200mm）
    EXPECT_TRUE(data.setLowerSensors(150.0, 150.0));
    EXPECT_FALSE(data.setLowerSensors(250.0, 150.0)); // 超出范围
    
    // 测试温度范围（-40到85°C）
    EXPECT_TRUE(data.setTemperature(25.0));
    EXPECT_FALSE(data.setTemperature(100.0));         // 超出范围
    EXPECT_FALSE(data.setTemperature(-50.0));         // 超出范围
    
    // 测试角度范围（-90到90度）
    EXPECT_TRUE(data.setMeasuredAngle(45.0));
    EXPECT_FALSE(data.setMeasuredAngle(100.0));       // 超出范围
    EXPECT_FALSE(data.setMeasuredAngle(-100.0));      // 超出范围
}