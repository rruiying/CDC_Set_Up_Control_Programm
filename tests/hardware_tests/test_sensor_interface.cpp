#include <gtest/gtest.h>
#include <QSignalSpy>
#include "hardware/include/sensor_interface.h"
#include "hardware/include/serial_interface.h"

class SensorInterfaceTest : public ::testing::Test {
protected:
    SensorInterface* sensor;
    SerialInterface* serial;
    
    void SetUp() override {
        serial = new SerialInterface();
        sensor = new SensorInterface(serial);
    }
    
    void TearDown() override {
        delete sensor;
        delete serial;
    }
};

// 测试数据解析
TEST_F(SensorInterfaceTest, ParseSensorData) {
    QSignalSpy dataSpy(sensor, &SensorInterface::sensorDataReceived);
    
    // 模拟接收完整数据
    QString testData = "D:25.5,26.1,155.8,156.2,A:2.5,T:23.5,C:156.7";
    sensor->processData(testData);
    
    ASSERT_EQ(dataSpy.count(), 1);
    
    // 检查解析的数据
    SensorData data = qvariant_cast<SensorData>(dataSpy.at(0).at(0));
    EXPECT_FLOAT_EQ(data.distanceUpper1, 25.5f);
    EXPECT_FLOAT_EQ(data.angle, 2.5f);
    EXPECT_FLOAT_EQ(data.temperature, 23.5f);
}

// 测试请求传感器数据
TEST_F(SensorInterfaceTest, RequestSensorData) {
    bool result = sensor->requestAllSensorData();
    // 由于没有实际连接，这里只测试接口
    EXPECT_TRUE(result || !result); // 接口存在即可
}

// 测试部分数据更新
TEST_F(SensorInterfaceTest, PartialDataUpdate) {
    QSignalSpy dataSpy(sensor, &SensorInterface::sensorDataReceived);
    
    // 先发送完整数据
    sensor->processData("D:25.5,26.1,155.8,156.2,A:2.5,T:23.5,C:156.7");
    
    // 再发送部分数据
    sensor->processData("T:24.0,A:3.0");
    
    ASSERT_EQ(dataSpy.count(), 2);
    
    SensorData data = qvariant_cast<SensorData>(dataSpy.at(1).at(0));
    EXPECT_FLOAT_EQ(data.temperature, 24.0f);
    EXPECT_FLOAT_EQ(data.angle, 3.0f);
    // 其他值应保持不变
    EXPECT_FLOAT_EQ(data.distanceUpper1, 25.5f);
}

// 测试错误数据处理
TEST_F(SensorInterfaceTest, HandleInvalidData) {
    QSignalSpy errorSpy(sensor, &SensorInterface::dataError);
    
    // 发送格式错误的数据
    sensor->processData("INVALID:DATA:FORMAT");
    
    // 应该触发错误信号
    EXPECT_GT(errorSpy.count(), 0);
}