// src/models/include/device_info.h
#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <string>
#include <chrono>
#include <cstdint>

/**
 * @brief 设备类型枚举
 */
enum class DeviceType {
    UNKNOWN,
    MOTOR_CONTROLLER,
    SENSOR,
    COMBINED  // 既有电机控制又有传感器的设备
};

/**
 * @brief 连接状态枚举
 */
enum class ConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

class DeviceInfo {
public:
    /**
     * @brief 默认构造函数
     */
    DeviceInfo();
    
    /**
     * @brief 带参数的构造函数
     * @param name 设备名称
     * @param portName 端口名称
     * @param baudRate 波特率
     */
    DeviceInfo(const std::string& name, const std::string& portName, int baudRate);
    
    /**
     * @brief 拷贝构造函数
     */
    DeviceInfo(const DeviceInfo& other);
    
    /**
     * @brief 赋值运算符
     */
    DeviceInfo& operator=(const DeviceInfo& other);
    
    // 基本信息的getter/setter
    std::string getName() const { return name; }
    void setName(const std::string& n) { name = n; }
    
    std::string getPortName() const { return portName; }
    void setPortName(const std::string& port) { portName = port; }
    
    int getBaudRate() const { return baudRate; }
    void setBaudRate(int rate) { baudRate = rate; }
    
    std::string getDeviceId() const { return deviceId; }
    
    // 设备类型
    DeviceType getDeviceType() const { return deviceType; }
    void setDeviceType(DeviceType type) { deviceType = type; }
    std::string getDeviceTypeString() const;
    
    // 连接状态
    ConnectionStatus getConnectionStatus() const { return connectionStatus; }
    void setConnectionStatus(ConnectionStatus status);
    std::string getConnectionStatusString() const;
    bool isConnected() const { return connectionStatus == ConnectionStatus::CONNECTED; }
    
    // 时间统计
    int64_t getLastConnectTime() const { return lastConnectTime; }
    int64_t getLastDisconnectTime() const { return lastDisconnectTime; }
    int64_t getLastActivityTime() const { return lastActivityTime; }
    void updateLastActivityTime();
    
    // 连接统计
    int getConnectionCount() const { return connectionCount; }
    int getDisconnectionCount() const { return disconnectionCount; }
    int64_t getTotalConnectedTime() const;
    
    // 错误统计
    int getErrorCount() const { return errorCount; }
    int64_t getLastErrorTime() const { return lastErrorTime; }
    std::string getLastErrorMessage() const { return lastErrorMessage; }
    void recordError(const std::string& errorMsg);
    void clearErrorStatistics();
    
    // 序列化
    std::string serialize() const;
    bool deserialize(const std::string& data);
    
    // 实用方法
    bool isSamePort(const DeviceInfo& other) const;
    std::string getSummary() const;
    
private:
    // 基本信息
    std::string name;
    std::string portName;
    int baudRate;
    std::string deviceId;
    DeviceType deviceType;
    
    // 连接状态
    ConnectionStatus connectionStatus;
    int64_t lastConnectTime;      // 最后连接时间（毫秒时间戳）
    int64_t lastDisconnectTime;   // 最后断开时间
    int64_t lastActivityTime;     // 最后活动时间
    int64_t currentSessionStart;  // 当前会话开始时间
    
    // 统计信息
    int connectionCount;
    int disconnectionCount;
    int64_t totalConnectedTime;   // 总连接时间（毫秒）
    
    // 错误信息
    int errorCount;
    int64_t lastErrorTime;
    std::string lastErrorMessage;
    
    // 辅助方法
    void generateDeviceId();
    DeviceType inferDeviceType() const;
    int64_t getCurrentTimestamp() const;
};

#endif // DEVICE_INFO_H