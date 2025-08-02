// tests/core_tests/test_sensor_manager.cpp
#include <gtest/gtest.h>
#include "core/include/sensor_manager.h"
#include "hardware/include/serial_interface.h"
#include "hardware/include/command_protocol.h"
#include "models/include/system_config.h"
#include "utils/include/logger.h"
#include <thread>
#include <chrono>

// 模拟串口用于测试
class MockSerialForSensor : public SerialInterface {
public:
    MockSerialForSensor() {
        setMockMode(true);
    }
    
    void simulateSensorResponse() {
        // 模拟传感器数据响应
        mockResponses.push_back("SENSORS:12.5,13.0,156.2,156.8,23.5,2.5,157.3\r\n");
    }
    
    void simulateErrorResponse() {
        mockResponses.push_back("ERROR:SENSOR_READ_FAILED\r\n");
    }
    
    void simulateTimeout() {
        // 不添加任何响应，导致超时
    }
    
    std::queue<std::string> mockResponses;
};

class SensorManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setConsoleOutput(false);
        SystemConfig::getInstance().reset();
        
        mockSerial = std::make_shared<MockSerialForSensor>();
        sensorManager = std::make_unique<SensorManager>(mockSerial);
    }
    
    void TearDown() override {
        if (sensorManager->isRunning()) {
            sensorManager->stop();
        }
    }
    
    std::shared_ptr<MockSerialForSensor> mockSerial;
    std::unique_ptr<SensorManager> sensorManager;
};

// 测试初始化
TEST_F(SensorManagerTest, Initialization) {
    EXPECT_FALSE(sensorManager->isRunning());
    EXPECT_FALSE(sensorManager->hasValidData());
    EXPECT_EQ(sensorManager->getUpdateInterval(), 
              SystemConfig::getInstance().getSensorUpdateInterval());
}

// 测试启动和停止
TEST_F(SensorManagerTest, StartAndStop) {
    // 设置更新间隔
    sensorManager->setUpdateInterval(100); // 100ms
    
    // 启动
    EXPECT_TRUE(sensorManager->start());
    EXPECT_TRUE(sensorManager->isRunning());
    
    // 等待一下
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 停止
    sensorManager->stop();
    EXPECT_FALSE(sensorManager->isRunning());
}

// 测试重复启动
TEST_F(SensorManagerTest, DoubleStart) {
    EXPECT_TRUE(sensorManager->start());
    EXPECT_FALSE(sensorManager->start()); // 第二次启动应该失败
}

// 测试单次读取
TEST_F(SensorManagerTest, SingleRead) {
    // 准备模拟响应
    mockSerial->simulateSensorResponse();
    
    // 执行单次读取
    bool success = sensorManager->readSensorsOnce();
    EXPECT_TRUE(success);
    EXPECT_TRUE(sensorManager->hasValidData());
    
    // 获取数据
    auto data = sensorManager->getLatestData();
    EXPECT_DOUBLE_EQ(data.getUpperSensor1(), 12.5);
    EXPECT_DOUBLE_EQ(data.getUpperSensor2(), 13.0);
    EXPECT_DOUBLE_EQ(data.getTemperature(), 23.5);
}

// 测试读取失败
TEST_F(SensorManagerTest, ReadFailure) {
    // 模拟错误响应
    mockSerial->simulateErrorResponse();
    
    bool success = sensorManager->readSensorsOnce();
    EXPECT_FALSE(success);
    EXPECT_FALSE(sensorManager->hasValidData());
}

// 测试超时处理
TEST_F(SensorManagerTest, ReadTimeout) {
    // 设置短超时
    sensorManager->setReadTimeout(100);
    
    // 不提供响应，导致超时
    mockSerial->simulateTimeout();
    
    auto start = std::chrono::steady_clock::now();
    bool success = sensorManager->readSensorsOnce();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    
    EXPECT_FALSE(success);
    EXPECT_GE(duration, 90); // 至少90ms
    EXPECT_LE(duration, 150); // 最多150ms
}

// 测试自动更新
TEST_F(SensorManagerTest, AutoUpdate) {
    // 设置短更新间隔
    sensorManager->setUpdateInterval(50); // 50ms
    
    // 准备多个响应
    for (int i = 0; i < 5; i++) {
        mockSerial->simulateSensorResponse();
    }
    
    // 启动自动更新
    sensorManager->start();
    
    // 等待几个更新周期
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // 停止
    sensorManager->stop();
    
    // 检查是否有数据
    EXPECT_TRUE(sensorManager->hasValidData());
    
    // 检查读取次数
    EXPECT_GE(sensorManager->getReadCount(), 4);
}

// 测试数据回调
TEST_F(SensorManagerTest, DataCallback) {
    bool callbackCalled = false;
    SensorData receivedData;
    
    // 设置回调
    sensorManager->setDataCallback([&](const SensorData& data) {
        callbackCalled = true;
        receivedData = data;
    });
    
    // 模拟响应并读取
    mockSerial->simulateSensorResponse();
    sensorManager->readSensorsOnce();
    
    // 验证回调被调用
    EXPECT_TRUE(callbackCalled);
    EXPECT_DOUBLE_EQ(receivedData.getUpperSensor1(), 12.5);
}

// 测试错误回调
TEST_F(SensorManagerTest, ErrorCallback) {
    bool errorCallbackCalled = false;
    std::string errorMessage;
    
    // 设置错误回调
    sensorManager->setErrorCallback([&](const std::string& error) {
        errorCallbackCalled = true;
        errorMessage = error;
    });
    
    // 触发错误
    mockSerial->simulateErrorResponse();
    sensorManager->readSensorsOnce();
    
    EXPECT_TRUE(errorCallbackCalled);
    EXPECT_FALSE(errorMessage.empty());
}

// 测试数据历史
TEST_F(SensorManagerTest, DataHistory) {
    // 设置历史大小
    sensorManager->setHistorySize(5);
    
    // 读取多次数据
    for (int i = 0; i < 10; i++) {
        mockSerial->simulateSensorResponse();
        sensorManager->readSensorsOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 获取历史数据
    auto history = sensorManager->getDataHistory();
    
    // 应该只保留最近5条
    EXPECT_EQ(history.size(), 5);
}

// 测试数据平均值
TEST_F(SensorManagerTest, DataAverage) {
    // 准备不同的传感器数据
    mockSerial->mockResponses.push("SENSORS:10.0,10.0,150.0,150.0,20.0,0.0,100.0\r\n");
    mockSerial->mockResponses.push("SENSORS:12.0,12.0,152.0,152.0,22.0,1.0,102.0\r\n");
    mockSerial->mockResponses.push("SENSORS:14.0,14.0,154.0,154.0,24.0,2.0,104.0\r\n");
    
    // 读取数据
    for (int i = 0; i < 3; i++) {
        sensorManager->readSensorsOnce();
    }
    
    // 获取平均值
    auto average = sensorManager->getAverageData(3);
    
    // 验证平均值
    EXPECT_DOUBLE_EQ(average.getUpperSensor1(), 12.0); // (10+12+14)/3
    EXPECT_DOUBLE_EQ(average.getUpperSensor2(), 12.0);
    EXPECT_DOUBLE_EQ(average.getTemperature(), 22.0); // (20+22+24)/3
}

// 测试数据过滤
TEST_F(SensorManagerTest, DataFiltering) {
    // 启用数据过滤
    sensorManager->setFilteringEnabled(true);
    sensorManager->setFilterThreshold(50.0); // 50%变化视为异常
    
    // 正常数据
    mockSerial->mockResponses.push("SENSORS:10.0,10.0,150.0,150.0,20.0,0.0,100.0\r\n");
    sensorManager->readSensorsOnce();
    
    // 异常数据（变化太大）
    mockSerial->mockResponses.push("SENSORS:100.0,100.0,150.0,150.0,20.0,0.0,100.0\r\n");
    sensorManager->readSensorsOnce();
    
    // 获取最新数据，应该是第一次的数据（异常数据被过滤）
    auto data = sensorManager->getLatestData();
    EXPECT_DOUBLE_EQ(data.getUpperSensor1(), 10.0);
}

// 测试统计信息
TEST_F(SensorManagerTest, Statistics) {
    // 成功读取
    for (int i = 0; i < 3; i++) {
        mockSerial->simulateSensorResponse();
        sensorManager->readSensorsOnce();
    }
    
    // 失败读取
    for (int i = 0; i < 2; i++) {
        mockSerial->simulateErrorResponse();
        sensorManager->readSensorsOnce();
    }
    
    // 获取统计
    auto stats = sensorManager->getStatistics();
    
    EXPECT_EQ(stats.totalReads, 5);
    EXPECT_EQ(stats.successfulReads, 3);
    EXPECT_EQ(stats.failedReads, 2);
    EXPECT_DOUBLE_EQ(stats.successRate, 60.0); // 3/5 = 60%
}

// 测试暂停和恢复
TEST_F(SensorManagerTest, PauseAndResume) {
    sensorManager->setUpdateInterval(50);
    
    // 准备响应
    for (int i = 0; i < 10; i++) {
        mockSerial->simulateSensorResponse();
    }
    
    // 启动
    sensorManager->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto countBefore = sensorManager->getReadCount();
    
    // 暂停
    sensorManager->pause();
    EXPECT_TRUE(sensorManager->isPaused());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto countDuringPause = sensorManager->getReadCount();
    
    // 暂停期间不应该有新的读取
    EXPECT_EQ(countBefore, countDuringPause);
    
    // 恢复
    sensorManager->resume();
    EXPECT_FALSE(sensorManager->isPaused());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto countAfter = sensorManager->getReadCount();
    
    // 恢复后应该继续读取
    EXPECT_GT(countAfter, countDuringPause);
    
    sensorManager->stop();
}

// 测试重置
TEST_F(SensorManagerTest, Reset) {
    // 读取一些数据
    for (int i = 0; i < 5; i++) {
        mockSerial->simulateSensorResponse();
        sensorManager->readSensorsOnce();
    }
    
    EXPECT_TRUE(sensorManager->hasValidData());
    EXPECT_GT(sensorManager->getReadCount(), 0);
    
    // 重置
    sensorManager->reset();
    
    EXPECT_FALSE(sensorManager->hasValidData());
    EXPECT_EQ(sensorManager->getReadCount(), 0);
    
    auto stats = sensorManager->getStatistics();
    EXPECT_EQ(stats.totalReads, 0);
}

// 测试线程安全
TEST_F(SensorManagerTest, ThreadSafety) {
    const int numThreads = 5;
    const int readsPerThread = 20;
    std::vector<std::thread> threads;
    
    // 准备足够的响应
    for (int i = 0; i < numThreads * readsPerThread; i++) {
        mockSerial->simulateSensorResponse();
    }
    
    // 启动多个线程读取数据
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([this]() {
            for (int j = 0; j < readsPerThread; j++) {
                sensorManager->readSensorsOnce();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 验证读取次数
    EXPECT_EQ(sensorManager->getReadCount(), numThreads * readsPerThread);
}