// tests/models_tests/test_device_info.cpp
#include <gtest/gtest.h>
#include "models/include/device_info.h"
#include <chrono>
#include <thread>

class DeviceInfoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前的初始化
    }
};

// 测试默认构造函数
TEST_F(DeviceInfoTest, DefaultConstructor) {
    DeviceInfo device;
    
    EXPECT_TRUE(device.getName().empty());
    EXPECT_TRUE(device.getPortName().empty());
    EXPECT_EQ(device.getBaudRate(), 115200); // 默认波特率
    EXPECT_FALSE(device.isConnected());
    EXPECT_EQ(device.getDeviceType(), DeviceType::UNKNOWN);
}

// 测试带参数构造函数
TEST_F(DeviceInfoTest, ParameterizedConstructor) {
    DeviceInfo device("Motor Controller", "COM3", 9600);
    
    EXPECT_EQ(device.getName(), "Motor Controller");
    EXPECT_EQ(device.getPortName(), "COM3");
    EXPECT_EQ(device.getBaudRate(), 9600);
    EXPECT_FALSE(device.isConnected()); // 初始状态未连接
    EXPECT_EQ(device.getDeviceType(), DeviceType::MOTOR_CONTROLLER); // 根据名称推断
}

// 测试设备类型推断
TEST_F(DeviceInfoTest, DeviceTypeInference) {
    DeviceInfo motor("Motor Controller", "COM3", 115200);
    EXPECT_EQ(motor.getDeviceType(), DeviceType::MOTOR_CONTROLLER);
    
    DeviceInfo sensor("Capacitance Sensor", "COM4", 9600);
    EXPECT_EQ(sensor.getDeviceType(), DeviceType::SENSOR);
    
    DeviceInfo combined("CDC System", "COM5", 115200);
    EXPECT_EQ(combined.getDeviceType(), DeviceType::COMBINED);
    
    DeviceInfo unknown("Some Device", "COM6", 115200);
    EXPECT_EQ(unknown.getDeviceType(), DeviceType::UNKNOWN);
}

// 测试设备类型手动设置
TEST_F(DeviceInfoTest, SetDeviceType) {
    DeviceInfo device("Custom Device", "COM3", 115200);
    
    device.setDeviceType(DeviceType::SENSOR);
    EXPECT_EQ(device.getDeviceType(), DeviceType::SENSOR);
    EXPECT_EQ(device.getDeviceTypeString(), "Sensor");
}

// 测试连接状态管理
TEST_F(DeviceInfoTest, ConnectionStatus) {
    DeviceInfo device("Test Device", "COM3", 115200);
    
    EXPECT_FALSE(device.isConnected());
    EXPECT_EQ(device.getConnectionStatus(), ConnectionStatus::DISCONNECTED);
    
    // 设置为连接中
    device.setConnectionStatus(ConnectionStatus::CONNECTING);
    EXPECT_FALSE(device.isConnected());
    EXPECT_EQ(device.getConnectionStatus(), ConnectionStatus::CONNECTING);
    
    // 设置为已连接
    device.setConnectionStatus(ConnectionStatus::CONNECTED);
    EXPECT_TRUE(device.isConnected());
    EXPECT_EQ(device.getConnectionStatus(), ConnectionStatus::CONNECTED);
    
    // 记录连接时间
    auto connectTime = device.getLastConnectTime();
    EXPECT_GT(connectTime, 0);
    
    // 设置为断开
    device.setConnectionStatus(ConnectionStatus::DISCONNECTED);
    EXPECT_FALSE(device.isConnected());
    
    // 记录断开时间
    auto disconnectTime = device.getLastDisconnectTime();
    EXPECT_GT(disconnectTime, connectTime);
}

// 测试连接统计
TEST_F(DeviceInfoTest, ConnectionStatistics) {
    DeviceInfo device("Test Device", "COM3", 115200);
    
    // 初始统计
    EXPECT_EQ(device.getConnectionCount(), 0);
    EXPECT_EQ(device.getDisconnectionCount(), 0);
    EXPECT_EQ(device.getTotalConnectedTime(), 0);
    
    // 模拟连接
    device.setConnectionStatus(ConnectionStatus::CONNECTED);
    EXPECT_EQ(device.getConnectionCount(), 1);
    
    // 等待一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 断开连接
    device.setConnectionStatus(ConnectionStatus::DISCONNECTED);
    EXPECT_EQ(device.getDisconnectionCount(), 1);
    
    // 检查连接时间
    auto totalTime = device.getTotalConnectedTime();
    EXPECT_GE(totalTime, 100);
    EXPECT_LE(totalTime, 200); // 允许一些误差
    
    // 再次连接
    device.setConnectionStatus(ConnectionStatus::CONNECTED);
    EXPECT_EQ(device.getConnectionCount(), 2);
}

// 测试最后活动时间
TEST_F(DeviceInfoTest, LastActivityTime) {
    DeviceInfo device("Test Device", "COM3", 115200);
    
    auto initialTime = device.getLastActivityTime();
    
    // 更新活动时间
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    device.updateLastActivityTime();
    
    auto newTime = device.getLastActivityTime();
    EXPECT_GT(newTime, initialTime);
}

// 测试设备信息更新
TEST_F(DeviceInfoTest, UpdateDeviceInfo) {
    DeviceInfo device("Old Name", "COM3", 9600);
    
    device.setName("New Name");
    EXPECT_EQ(device.getName(), "New Name");
    
    device.setPortName("COM4");
    EXPECT_EQ(device.getPortName(), "COM4");
    
    device.setBaudRate(115200);
    EXPECT_EQ(device.getBaudRate(), 115200);
}

// 测试设备ID
TEST_F(DeviceInfoTest, DeviceID) {
    DeviceInfo device1("Device1", "COM3", 115200);
    DeviceInfo device2("Device2", "COM4", 115200);
    
    // 每个设备应该有唯一的ID
    EXPECT_NE(device1.getDeviceId(), device2.getDeviceId());
    
    // ID应该是非空字符串
    EXPECT_FALSE(device1.getDeviceId().empty());
    EXPECT_FALSE(device2.getDeviceId().empty());
}

// 测试设备比较
TEST_F(DeviceInfoTest, DeviceComparison) {
    DeviceInfo device1("Device", "COM3", 115200);
    DeviceInfo device2("Device", "COM3", 115200);
    DeviceInfo device3("Device", "COM4", 115200);
    
    // 相同端口的设备应该被认为是相同的
    EXPECT_TRUE(device1.isSamePort(device2));
    EXPECT_FALSE(device1.isSamePort(device3));
}

// 测试序列化和反序列化
TEST_F(DeviceInfoTest, Serialization) {
    DeviceInfo original("Test Device", "COM3", 9600);
    original.setDeviceType(DeviceType::MOTOR_CONTROLLER);
    original.setConnectionStatus(ConnectionStatus::CONNECTED);
    
    // 序列化为字符串
    std::string serialized = original.serialize();
    EXPECT_FALSE(serialized.empty());
    
    // 反序列化
    DeviceInfo restored;
    EXPECT_TRUE(restored.deserialize(serialized));
    
    // 验证恢复的数据
    EXPECT_EQ(restored.getName(), original.getName());
    EXPECT_EQ(restored.getPortName(), original.getPortName());
    EXPECT_EQ(restored.getBaudRate(), original.getBaudRate());
    EXPECT_EQ(restored.getDeviceType(), original.getDeviceType());
}

// 测试错误统计
TEST_F(DeviceInfoTest, ErrorStatistics) {
    DeviceInfo device("Test Device", "COM3", 115200);
    
    EXPECT_EQ(device.getErrorCount(), 0);
    EXPECT_EQ(device.getLastErrorTime(), 0);
    EXPECT_TRUE(device.getLastErrorMessage().empty());
    
    // 记录错误
    device.recordError("Communication timeout");
    EXPECT_EQ(device.getErrorCount(), 1);
    EXPECT_GT(device.getLastErrorTime(), 0);
    EXPECT_EQ(device.getLastErrorMessage(), "Communication timeout");
    
    // 记录另一个错误
    device.recordError("Invalid response");
    EXPECT_EQ(device.getErrorCount(), 2);
    EXPECT_EQ(device.getLastErrorMessage(), "Invalid response");
    
    // 清除错误统计
    device.clearErrorStatistics();
    EXPECT_EQ(device.getErrorCount(), 0);
    EXPECT_TRUE(device.getLastErrorMessage().empty());
}

// 测试设备状态字符串
TEST_F(DeviceInfoTest, StatusStrings) {
    DeviceInfo device("Test Device", "COM3", 115200);
    
    // 连接状态字符串
    device.setConnectionStatus(ConnectionStatus::DISCONNECTED);
    EXPECT_EQ(device.getConnectionStatusString(), "Disconnected");
    
    device.setConnectionStatus(ConnectionStatus::CONNECTING);
    EXPECT_EQ(device.getConnectionStatusString(), "Connecting");
    
    device.setConnectionStatus(ConnectionStatus::CONNECTED);
    EXPECT_EQ(device.getConnectionStatusString(), "Connected");
    
    device.setConnectionStatus(ConnectionStatus::ERROR);
    EXPECT_EQ(device.getConnectionStatusString(), "Error");
    
    // 设备类型字符串
    device.setDeviceType(DeviceType::MOTOR_CONTROLLER);
    EXPECT_EQ(device.getDeviceTypeString(), "Motor Controller");
    
    device.setDeviceType(DeviceType::SENSOR);
    EXPECT_EQ(device.getDeviceTypeString(), "Sensor");
}

// 测试设备信息摘要
TEST_F(DeviceInfoTest, DeviceInfoSummary) {
    DeviceInfo device("Motor Controller", "COM3", 115200);
    device.setConnectionStatus(ConnectionStatus::CONNECTED);
    device.setDeviceType(DeviceType::MOTOR_CONTROLLER);
    
    std::string summary = device.getSummary();
    
    // 摘要应该包含关键信息
    EXPECT_TRUE(summary.find("Motor Controller") != std::string::npos);
    EXPECT_TRUE(summary.find("COM3") != std::string::npos);
    EXPECT_TRUE(summary.find("115200") != std::string::npos);
    EXPECT_TRUE(summary.find("Connected") != std::string::npos);
}

// 测试拷贝和赋值
TEST_F(DeviceInfoTest, CopyAndAssignment) {
    DeviceInfo original("Test Device", "COM3", 9600);
    original.setConnectionStatus(ConnectionStatus::CONNECTED);
    original.recordError("Test error");
    
    // 拷贝构造
    DeviceInfo copy(original);
    EXPECT_EQ(copy.getName(), original.getName());
    EXPECT_EQ(copy.getPortName(), original.getPortName());
    EXPECT_EQ(copy.getErrorCount(), original.getErrorCount());
    
    // 赋值操作
    DeviceInfo assigned;
    assigned = original;
    EXPECT_EQ(assigned.getName(), original.getName());
    EXPECT_EQ(assigned.getConnectionStatus(), original.getConnectionStatus());
}

// 测试设备列表管理
TEST_F(DeviceInfoTest, DeviceListManagement) {
    std::vector<DeviceInfo> devices;
    
    devices.emplace_back("Motor Controller", "COM3", 115200);
    devices.emplace_back("Sensor 1", "COM4", 9600);
    devices.emplace_back("Sensor 2", "COM5", 9600);
    
    // 查找特定端口的设备
    auto it = std::find_if(devices.begin(), devices.end(),
        [](const DeviceInfo& d) { return d.getPortName() == "COM4"; });
    
    EXPECT_NE(it, devices.end());
    EXPECT_EQ(it->getName(), "Sensor 1");
    
    // 统计连接的设备
    for (auto& device : devices) {
        device.setConnectionStatus(ConnectionStatus::CONNECTED);
    }
    
    auto connectedCount = std::count_if(devices.begin(), devices.end(),
        [](const DeviceInfo& d) { return d.isConnected(); });
    
    EXPECT_EQ(connectedCount, 3);
}