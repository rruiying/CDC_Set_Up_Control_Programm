// tests/models_tests/test_measurement_data.cpp
#include <gtest/gtest.h>
#include "models/include/measurement_data.h"
#include "models/include/sensor_data.h"
#include <chrono>
#include <thread>
#include <cmath>

class MeasurementDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试用的传感器数据
        testSensorData = SensorData(12.5, 13.0, 156.2, 156.8, 23.5, 2.5, 157.3);
    }
    
    SensorData testSensorData;
};

// 测试默认构造函数
TEST_F(MeasurementDataTest, DefaultConstructor) {
    MeasurementData data;
    
    // 验证时间戳不为0（应该是当前时间）
    EXPECT_GT(data.getTimestamp(), 0);
    
    // 验证默认值
    EXPECT_DOUBLE_EQ(data.getSetHeight(), 0.0);
    EXPECT_DOUBLE_EQ(data.getSetAngle(), 0.0);
    EXPECT_DOUBLE_EQ(data.getTheoreticalCapacitance(), 0.0);
    
    // 传感器数据应该是默认构造的
    const SensorData& sensorData = data.getSensorData();
    EXPECT_DOUBLE_EQ(sensorData.getUpperSensor1(), 0.0);
}

// 测试带参数的构造函数
TEST_F(MeasurementDataTest, ParameterizedConstructor) {
    double setHeight = 25.0;
    double setAngle = 5.5;
    
    MeasurementData data(setHeight, setAngle, testSensorData);
    
    EXPECT_DOUBLE_EQ(data.getSetHeight(), setHeight);
    EXPECT_DOUBLE_EQ(data.getSetAngle(), setAngle);
    
    // 验证传感器数据被正确复制
    const SensorData& sensorData = data.getSensorData();
    EXPECT_DOUBLE_EQ(sensorData.getUpperSensor1(), 12.5);
    EXPECT_DOUBLE_EQ(sensorData.getTemperature(), 23.5);
}

// 测试理论电容计算
TEST_F(MeasurementDataTest, TheoreticalCapacitanceCalculation) {
    double setHeight = 10.0;  // 10mm = 0.01m
    double setAngle = 0.0;    // 无倾斜
    
    MeasurementData data(setHeight, setAngle, testSensorData);
    
    // 设置电容板参数
    data.setPlateArea(0.01);  // 0.01 m² = 100 cm²
    data.setDielectricConstant(1.0);  // 空气
    
    // C = ε₀ * εᵣ * A / d
    // ε₀ = 8.854e-12 F/m
    // 预期值 = 8.854e-12 * 1.0 * 0.01 / 0.01 = 8.854e-12 F = 8.854 pF
    double expected = 8.854;
    EXPECT_NEAR(data.getTheoreticalCapacitance(), expected, 0.01);
}

// 测试带角度的理论电容计算
TEST_F(MeasurementDataTest, TheoreticalCapacitanceWithAngle) {
    double setHeight = 10.0;  // 10mm
    double setAngle = 45.0;   // 45度倾斜
    
    MeasurementData data(setHeight, setAngle, testSensorData);
    data.setPlateArea(0.01);
    data.setDielectricConstant(1.0);
    
    // 倾斜时有效面积减少：A_eff = A * cos(angle)
    double angleRad = setAngle * M_PI / 180.0;
    double effectiveArea = 0.01 * std::cos(angleRad);
    double expected = 8.854e-12 * 1.0 * effectiveArea / 0.01 * 1e12;
    
    EXPECT_NEAR(data.getTheoreticalCapacitance(), expected, 0.01);
}

// 测试电容差值计算
TEST_F(MeasurementDataTest, CapacitanceDifference) {
    MeasurementData data(25.0, 0.0, testSensorData);
    data.setPlateArea(0.01);
    
    double theoretical = data.getTheoreticalCapacitance();
    double measured = testSensorData.getMeasuredCapacitance();
    double difference = data.getCapacitanceDifference();
    
    EXPECT_DOUBLE_EQ(difference, measured - theoretical);
}

// 测试时间戳
TEST_F(MeasurementDataTest, TimestampHandling) {
    MeasurementData data1;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    MeasurementData data2;
    
    // data2的时间戳应该晚于data1
    EXPECT_GT(data2.getTimestamp(), data1.getTimestamp());
    
    // 测试手动设置时间戳
    int64_t customTime = 1234567890;
    data1.setTimestamp(customTime);
    EXPECT_EQ(data1.getTimestamp(), customTime);
}

// 测试数据验证
TEST_F(MeasurementDataTest, DataValidation) {
    // 有效数据
    MeasurementData validData(25.0, 5.0, testSensorData);
    EXPECT_TRUE(validData.isValid());
    
    // 无效的设定高度
    MeasurementData invalidHeight(-10.0, 5.0, testSensorData);
    EXPECT_FALSE(invalidHeight.isValid());
    
    // 无效的设定角度
    MeasurementData invalidAngle(25.0, 100.0, testSensorData);
    EXPECT_FALSE(invalidAngle.isValid());
    
    // 无效的传感器数据
    SensorData invalidSensorData;  // 默认构造的传感器数据是无效的
    MeasurementData invalidSensor(25.0, 5.0, invalidSensorData);
    EXPECT_FALSE(invalidSensor.isValid());
}

// 测试CSV导出
TEST_F(MeasurementDataTest, CSVExport) {
    MeasurementData data(25.0, 5.5, testSensorData);
    data.setPlateArea(0.01);
    
    std::string csv = data.toCSV();
    
    // 验证CSV包含所有必要字段
    EXPECT_TRUE(csv.find("25.0") != std::string::npos);    // 设定高度
    EXPECT_TRUE(csv.find("5.5") != std::string::npos);     // 设定角度
    EXPECT_TRUE(csv.find("12.5") != std::string::npos);    // 传感器数据
}

// 测试CSV头部
TEST_F(MeasurementDataTest, CSVHeader) {
    std::string header = MeasurementData::getCSVHeader();
    
    // 验证包含所有必要的列名
    EXPECT_TRUE(header.find("Timestamp") != std::string::npos);
    EXPECT_TRUE(header.find("Set_Height") != std::string::npos);
    EXPECT_TRUE(header.find("Set_Angle") != std::string::npos);
    EXPECT_TRUE(header.find("Theoretical_Capacitance") != std::string::npos);
    EXPECT_TRUE(header.find("Capacitance_Difference") != std::string::npos);
    
    // 也应该包含传感器数据的列名
    EXPECT_TRUE(header.find("Upper_Sensor_1") != std::string::npos);
}

// 测试拷贝构造和赋值
TEST_F(MeasurementDataTest, CopyOperations) {
    MeasurementData original(30.0, 7.5, testSensorData);
    original.setPlateArea(0.015);
    original.setDielectricConstant(1.5);
    
    // 拷贝构造
    MeasurementData copy(original);
    EXPECT_DOUBLE_EQ(copy.getSetHeight(), original.getSetHeight());
    EXPECT_DOUBLE_EQ(copy.getSetAngle(), original.getSetAngle());
    EXPECT_DOUBLE_EQ(copy.getPlateArea(), original.getPlateArea());
    EXPECT_EQ(copy.getTimestamp(), original.getTimestamp());
    
    // 赋值操作
    MeasurementData assigned;
    assigned = original;
    EXPECT_DOUBLE_EQ(assigned.getSetHeight(), original.getSetHeight());
    EXPECT_DOUBLE_EQ(assigned.getDielectricConstant(), original.getDielectricConstant());
}

// 测试时间格式化
TEST_F(MeasurementDataTest, TimeFormatting) {
    MeasurementData data;
    
    // 设置一个已知的时间戳 (2024-01-01 12:00:00)
    // Unix时间戳：1704110400000 (毫秒)
    data.setTimestamp(1704110400000);
    
    std::string timeStr = data.getFormattedTime();
    
    // 验证格式（应该包含日期和时间）
    EXPECT_TRUE(timeStr.find("2024") != std::string::npos);
    EXPECT_TRUE(timeStr.find(":") != std::string::npos);  // 时间分隔符
}

// 测试数据更新
TEST_F(MeasurementDataTest, DataUpdate) {
    MeasurementData data;
    
    // 更新设定值
    data.setHeight(35.0);
    data.setAngle(8.5);
    EXPECT_DOUBLE_EQ(data.getSetHeight(), 35.0);
    EXPECT_DOUBLE_EQ(data.getSetAngle(), 8.5);
    
    // 更新传感器数据
    SensorData newSensorData(15.0, 15.5, 160.0, 161.0, 24.0, 3.0, 160.0);
    data.updateSensorData(newSensorData);
    
    const SensorData& updated = data.getSensorData();
    EXPECT_DOUBLE_EQ(updated.getUpperSensor1(), 15.0);
    EXPECT_DOUBLE_EQ(updated.getTemperature(), 24.0);
}

// 测试安全范围检查
TEST_F(MeasurementDataTest, SafetyRangeCheck) {
    MeasurementData data;
    
    // 设置安全范围
    data.setSafetyLimits(0.0, 100.0, -45.0, 45.0);
    
    // 在范围内
    EXPECT_TRUE(data.setHeight(50.0));
    EXPECT_TRUE(data.setAngle(30.0));
    
    // 超出范围
    EXPECT_FALSE(data.setHeight(150.0));
    EXPECT_FALSE(data.setAngle(60.0));
    EXPECT_FALSE(data.setHeight(-10.0));
    EXPECT_FALSE(data.setAngle(-50.0));
}