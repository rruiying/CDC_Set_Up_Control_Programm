#include <gtest/gtest.h>
#include <QSignalSpy>
#include "core/include/sensor_manager.h"
#include "hardware/include/sensor_interface.h"

class MockSensorInterface : public SensorInterface {
public:
    MockSensorInterface() : SensorInterface(nullptr) {}
    
    MOCK_METHOD(bool, requestAllSensorData, (), (override));
    MOCK_METHOD(void, startPolling, (int), (override));
    MOCK_METHOD(void, stopPolling, (), (override));
};

class SensorManagerTest : public ::testing::Test {
protected:
    SensorManager* manager;
    MockSensorInterface* mockSensor;
    
    void SetUp() override {
        mockSensor = new MockSensorInterface();
        manager = new SensorManager(mockSensor);
    }
    
    void TearDown() override {
        delete manager;
    }
};

// 测试启动监控
TEST_F(SensorManagerTest, StartMonitoring) {
    EXPECT_CALL(*mockSensor, startPolling(100))
        .Times(1);
    
    manager->startMonitoring(100);
    EXPECT_TRUE(manager->isMonitoring());
}

// 测试停止监控
TEST_F(SensorManagerTest, StopMonitoring) {
    EXPECT_CALL(*mockSensor, stopPolling())
        .Times(1);
    
    manager->stopMonitoring();
    EXPECT_FALSE(manager->isMonitoring());
}

// 测试数据处理
TEST_F(SensorManagerTest, ProcessSensorData) {
    QSignalSpy updateSpy(manager, &SensorManager::dataUpdated);
    
    SensorData testData;
    testData.temperature = 25.0f;
    testData.distanceUpper1 = 30.0f;
    
    manager->processSensorData(testData);
    
    ASSERT_EQ(updateSpy.count(), 1);
    
    // 检查计算值
    auto current = manager->getCurrentData();
    EXPECT_EQ(current.temperature, 25.0f);
}

// 测试数据历史
TEST_F(SensorManagerTest, DataHistory) {
    // 添加多个数据点
    for (int i = 0; i < 5; ++i) {
        SensorData data;
        data.temperature = 20.0f + i;
        manager->processSensorData(data);
    }
    
    auto history = manager->getDataHistory(10);
    EXPECT_EQ(history.size(), 5);
}

// 测试数据统计
TEST_F(SensorManagerTest, DataStatistics) {
    // 添加测试数据
    for (int i = 0; i < 10; ++i) {
        SensorData data;
        data.temperature = 20.0f + (i % 3);
        manager->processSensorData(data);
    }
    
    auto stats = manager->getStatistics();
    EXPECT_GT(stats.avgTemperature, 0);
    EXPECT_GT(stats.dataPoints, 0);
}