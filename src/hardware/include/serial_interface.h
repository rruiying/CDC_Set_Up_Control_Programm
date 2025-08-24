#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>

struct SerialPortInfo {
    std::string portName;     // 端口名称（如 COM3, /dev/ttyUSB0）
    std::string description;  // 端口描述
    std::string hardwareId;   // 硬件ID
    bool isAvailable;         // 是否可用
};

enum class DataBits {
    FIVE = 5,
    SIX = 6,
    SEVEN = 7,
    EIGHT = 8
};

enum class Parity {
    NONE,
    ODD,
    EVEN,
    MARK,
    SPACE
};

enum class StopBits {
    ONE,
    ONE_POINT_FIVE,
    TWO
};

enum class FlowControl {
    NONE,
    HARDWARE,
    SOFTWARE
};

struct SerialPortConfig {
    int baudRate = 115200;
    DataBits dataBits = DataBits::EIGHT;
    Parity parity = Parity::NONE;
    StopBits stopBits = StopBits::ONE;
    FlowControl flowControl = FlowControl::NONE;
    int readTimeout = 1000;  // 毫秒
    int writeTimeout = 1000; // 毫秒
};

class SerialInterface {
public:
    SerialInterface();
    ~SerialInterface();
    
    // 回调函数类型
    using ConnectionCallback = std::function<void(bool connected)>;
    using DataReceivedCallback = std::function<void(const std::string& data)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    // 静态方法：获取可用端口列表
    static std::vector<SerialPortInfo> getAvailablePorts();
    
    // 连接管理
    bool open(const std::string& portName, int baudRate);
    bool open(const std::string& portName, const SerialPortConfig& config);
    void close();
    bool isOpen() const;
    
    // 端口信息
    std::string getCurrentPort() const { return currentPort; }
    int getCurrentBaudRate() const { return currentConfig.baudRate; }
    SerialPortConfig getCurrentConfig() const { return currentConfig; }
    
    // 数据收发
    bool sendCommand(const std::string& command);
    bool sendData(const std::vector<uint8_t>& data);
    std::string readLine(int timeoutMs = 1000);
    std::vector<uint8_t> readBytes(size_t count, int timeoutMs = 1000);
    int bytesAvailable() const;
    
    // 高级功能
    std::string sendAndReceive(const std::string& command, int timeoutMs = 5000);
    void flushBuffers();
    
    // 回调设置
    void setConnectionCallback(ConnectionCallback callback);
    void setDataReceivedCallback(DataReceivedCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    // 自动重连
    void setAutoReconnect(bool enable) { autoReconnect = enable; }
    bool isAutoReconnectEnabled() const { return autoReconnect; }
    
    // 测试支持
    void setMockMode(bool enable) { mockMode = enable; }
    bool isMockMode() const { return mockMode; }
    
protected:
    // 平台相关的实现（由子类或pimpl实现）
    virtual bool platformOpen(const std::string& portName, const SerialPortConfig& config);
    virtual void platformClose();
    virtual bool platformWrite(const std::vector<uint8_t>& data);
    virtual std::vector<uint8_t> platformRead(size_t maxBytes, int timeoutMs);
    virtual int platformBytesAvailable() const;
    virtual void platformFlush();
    
    // 模拟模式支持（用于测试）
    void simulateDisconnection();
    
private:
    // 内部方法
    void notifyConnection(bool connected);
    void notifyDataReceived(const std::string& data);
    void notifyError(const std::string& error);
    void reconnectThread();
    void receiveThread();
    std::string readUntilTerminator(const std::string& terminator, int timeoutMs);
    
    // 成员变量
    mutable std::mutex mutex;
    std::string currentPort;
    SerialPortConfig currentConfig;
    std::atomic<bool> connected{false};
    std::atomic<bool> autoReconnect{false};
    std::atomic<bool> mockMode{false};
    
    // 回调函数
    ConnectionCallback connectionCallback;
    DataReceivedCallback dataReceivedCallback;
    ErrorCallback errorCallback;
    
    // 重连线程
    std::unique_ptr<std::thread> reconnectThreadPtr;
    std::atomic<bool> stopReconnect{false};

    std::unique_ptr<std::thread> receiveThreadPtr;
    std::atomic<bool> stopReceiveThread{false};
    
    // 平台相关的实现指针（pimpl模式）
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // 测试支持
    friend class MockSerialPort;
};

#endif // SERIAL_INTERFACE_H