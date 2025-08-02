// tests/hardware_tests/test_serial_interface.cpp
#include <gtest/gtest.h>
#include "hardware/include/serial_interface.h"
#include "hardware/include/command_protocol.h"
#include "utils/include/logger.h"
#include <thread>
#include <chrono>

// 模拟串口类，用于测试
class MockSerialPort : public SerialInterface {
public:
    MockSerialPort() : SerialInterface() {
        // 设置为模拟模式
        setMockMode(true);
    }
    
    // 模拟响应
    void simulateResponse(const std::string& response) {
        mockResponses.push_back(response);
    }
    
    // 获取发送的命令（用于验证）
    std::vector<std::string> getSentCommands() const {
        return sentCommands;
    }
    
    void clearMockData() {
        mockResponses.clear();
        sentCommands.clear();
    }
    
private:
    std::vector<std::string> mockResponses;
    std::vector<std::string> sentCommands;
    
    // 友元类以访问私有成员
    friend class SerialInterface;
};

class SerialInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化日志系统（关闭输出）
        Logger::getInstance().setConsoleOutput(false);
        mockSerial = std::make_unique<MockSerialPort>();
    }
    
    void TearDown() override {
        if (mockSerial && mockSerial->isOpen()) {
            mockSerial->close();
        }
    }
    
    std::unique_ptr<MockSerialPort> mockSerial;
};

// 测试端口枚举
TEST_F(SerialInterfaceTest, GetAvailablePorts) {
    auto ports = SerialInterface::getAvailablePorts();
    
    // 在模拟模式下，应该返回一些模拟端口
    EXPECT_FALSE(ports.empty());
    
    // 验证端口信息结构
    if (!ports.empty()) {
        const auto& port = ports[0];
        EXPECT_FALSE(port.portName.empty());
        EXPECT_FALSE(port.description.empty());
    }
}

// 测试端口打开和关闭
TEST_F(SerialInterfaceTest, OpenAndClose) {
    // 打开模拟端口
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    EXPECT_TRUE(mockSerial->isOpen());
    EXPECT_EQ(mockSerial->getCurrentPort(), "COM_MOCK");
    EXPECT_EQ(mockSerial->getCurrentBaudRate(), 115200);
    
    // 关闭端口
    mockSerial->close();
    EXPECT_FALSE(mockSerial->isOpen());
    EXPECT_TRUE(mockSerial->getCurrentPort().empty());
}

// 测试重复打开
TEST_F(SerialInterfaceTest, OpenAlreadyOpenPort) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    // 尝试再次打开（应该先关闭旧的）
    EXPECT_TRUE(mockSerial->open("COM_MOCK2", 9600));
    EXPECT_EQ(mockSerial->getCurrentPort(), "COM_MOCK2");
    EXPECT_EQ(mockSerial->getCurrentBaudRate(), 9600);
}

// 测试发送命令
TEST_F(SerialInterfaceTest, SendCommand) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    // 发送命令
    std::string command = "SET_HEIGHT:25.0\r\n";
    EXPECT_TRUE(mockSerial->sendCommand(command));
    
    // 验证命令被记录
    auto sentCmds = mockSerial->getSentCommands();
    EXPECT_EQ(sentCmds.size(), 1);
    EXPECT_EQ(sentCmds[0], command);
}

// 测试接收数据
TEST_F(SerialInterfaceTest, ReceiveData) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    // 模拟接收数据
    std::string testData = "OK:HEIGHT_SET\r\n";
    mockSerial->simulateResponse(testData);
    
    // 读取数据
    std::string received = mockSerial->readLine(1000);
    EXPECT_EQ(received, testData);
}

// 测试超时
TEST_F(SerialInterfaceTest, ReadTimeout) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    // 不模拟任何响应，读取应该超时
    auto start = std::chrono::steady_clock::now();
    std::string received = mockSerial->readLine(100); // 100ms超时
    auto end = std::chrono::steady_clock::now();
    
    EXPECT_TRUE(received.empty());
    
    // 验证超时时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_GE(duration, 90);  // 至少90ms
    EXPECT_LE(duration, 150); // 最多150ms（留一些余量）
}

// 测试发送并等待响应
TEST_F(SerialInterfaceTest, SendAndReceive) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    // 设置模拟响应
    mockSerial->simulateResponse("OK:HEIGHT_SET\r\n");
    
    // 发送并接收
    std::string response = mockSerial->sendAndReceive("SET_HEIGHT:25.0\r\n", 1000);
    EXPECT_EQ(response, "OK:HEIGHT_SET\r\n");
}

// 测试命令队列
TEST_F(SerialInterfaceTest, CommandQueue) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    // 设置响应队列
    mockSerial->simulateResponse("OK:HEIGHT_SET\r\n");
    mockSerial->simulateResponse("OK:ANGLE_SET\r\n");
    mockSerial->simulateResponse("OK:MOVED\r\n");
    
    // 发送多个命令
    EXPECT_EQ(mockSerial->sendAndReceive("SET_HEIGHT:25.0\r\n", 1000), "OK:HEIGHT_SET\r\n");
    EXPECT_EQ(mockSerial->sendAndReceive("SET_ANGLE:5.5\r\n", 1000), "OK:ANGLE_SET\r\n");
    EXPECT_EQ(mockSerial->sendAndReceive("MOVE_TO:25.0,5.5\r\n", 1000), "OK:MOVED\r\n");
}

// 测试错误处理
TEST_F(SerialInterfaceTest, ErrorHandling) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    // 模拟错误响应
    mockSerial->simulateResponse("ERROR:OUT_OF_RANGE\r\n");
    
    std::string response = mockSerial->sendAndReceive("SET_HEIGHT:200.0\r\n", 1000);
    EXPECT_EQ(response, "ERROR:OUT_OF_RANGE\r\n");
}

// 测试缓冲区管理
TEST_F(SerialInterfaceTest, BufferManagement) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    // 清空缓冲区
    mockSerial->flushBuffers();
    
    // 验证缓冲区为空
    std::string data = mockSerial->readLine(10); // 短超时
    EXPECT_TRUE(data.empty());
}

// 测试多线程访问
TEST_F(SerialInterfaceTest, ThreadSafety) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    const int numThreads = 5;
    const int commandsPerThread = 20;
    std::vector<std::thread> threads;
    
    // 准备足够的模拟响应
    for (int i = 0; i < numThreads * commandsPerThread; ++i) {
        mockSerial->simulateResponse("OK\r\n");
    }
    
    // 启动多个线程发送命令
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i, commandsPerThread]() {
            for (int j = 0; j < commandsPerThread; ++j) {
                std::string cmd = "TEST_" + std::to_string(i) + "_" + std::to_string(j) + "\r\n";
                mockSerial->sendAndReceive(cmd, 100);
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 验证所有命令都被发送
    auto sentCmds = mockSerial->getSentCommands();
    EXPECT_EQ(sentCmds.size(), numThreads * commandsPerThread);
}

// 测试连接状态回调
TEST_F(SerialInterfaceTest, ConnectionCallback) {
    bool callbackCalled = false;
    bool connectionStatus = false;
    
    mockSerial->setConnectionCallback([&callbackCalled, &connectionStatus](bool connected) {
        callbackCalled = true;
        connectionStatus = connected;
    });
    
    // 打开端口应该触发回调
    mockSerial->open("COM_MOCK", 115200);
    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(connectionStatus);
    
    // 重置标志
    callbackCalled = false;
    
    // 关闭端口也应该触发回调
    mockSerial->close();
    EXPECT_TRUE(callbackCalled);
    EXPECT_FALSE(connectionStatus);
}

// 测试数据接收回调
TEST_F(SerialInterfaceTest, DataReceivedCallback) {
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    std::string receivedData;
    mockSerial->setDataReceivedCallback([&receivedData](const std::string& data) {
        receivedData = data;
    });
    
    // 模拟接收数据
    mockSerial->simulateResponse("SENSORS:1,2,3,4,5,6,7\r\n");
    
    // 读取数据（这应该触发回调）
    mockSerial->readLine(100);
    
    EXPECT_EQ(receivedData, "SENSORS:1,2,3,4,5,6,7\r\n");
}

// 测试端口配置
TEST_F(SerialInterfaceTest, PortConfiguration) {
    SerialPortConfig config;
    config.baudRate = 9600;
    config.dataBits = 8;
    config.parity = Parity::NONE;
    config.stopBits = StopBits::ONE;
    config.flowControl = FlowControl::NONE;
    
    EXPECT_TRUE(mockSerial->open("COM_MOCK", config));
    EXPECT_EQ(mockSerial->getCurrentBaudRate(), 9600);
}

// 测试自动重连
TEST_F(SerialInterfaceTest, AutoReconnect) {
    mockSerial->setAutoReconnect(true);
    EXPECT_TRUE(mockSerial->open("COM_MOCK", 115200));
    
    // 模拟连接丢失
    mockSerial->simulateDisconnection();
    
    // 等待一下，自动重连应该启动
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 应该重新连接
    EXPECT_TRUE(mockSerial->isOpen());
}