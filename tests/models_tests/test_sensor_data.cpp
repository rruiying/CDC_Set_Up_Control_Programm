#include <gtest/gtest.h>
#include "models/include/sensor_data.h"

class SensorDataTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试默认构造
TEST_F(SensorDataTest, DefaultConstruction) {
    SensorData data;
    
    EXPECT_FLOAT_EQ(data.distanceUpper1, 0.0f);
    EXPECT_FLOAT_EQ(data.distanceUpper2, 0.0f);
    EXPECT_FLOAT_EQ(data.temperature, 20.0f);  // 默认室温
    EXPECT_FALSE(data.isValid.temperature);
}

// 测试简单的数据解析
TEST_F(SensorDataTest, ParseSimpleData) {
    QString input = "T:25.5\n";
    SensorData data = SensorData::fromSerialData(input);
    
    EXPECT_FLOAT_EQ(data.temperature, 25.5f);
    EXPECT_TRUE(data.isValid.temperature);
}

// 测试完整数据解析
TEST_F(SensorDataTest, ParseCompleteData) {
    QString input = "D:25.5,26.1,155.8,156.2,A:2.5,T:23.5,C:156.7\n";
    SensorData data = SensorData::fromSerialData(input);
    
    EXPECT_FLOAT_EQ(data.distanceUpper1, 25.5f);
    EXPECT_FLOAT_EQ(data.angle, 2.5f);
    EXPECT_TRUE(data.isValid.distanceUpper1);
    EXPECT_TRUE(data.isValid.angle);
}

// 测试CSV导出
TEST_F(SensorDataTest, ExportToCSV) {
    SensorData data;
    data.temperature = 25.5f;
    data.angle = 2.5f;
    
    QString csv = data.toCSVLine();
    
    EXPECT_TRUE(csv.contains("25.5"));
    EXPECT_TRUE(csv.contains("2.5"));
}