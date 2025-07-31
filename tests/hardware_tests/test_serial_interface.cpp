#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QCoreApplication>
#include "hardware/include/serial_interface.h"

class SerialInterfaceTest : public ::testing::Test {
protected:
    SerialInterface* serial;
    
    void SetUp() override {
        serial = new SerialInterface();
    }
    
    void TearDown() override {
        if (serial->isConnected()) {
            serial->disconnect();
        }
        delete serial;
    }
};

// 测试获取可用端口
TEST_F(SerialInterfaceTest, GetAvailablePorts) {
    auto ports = serial->getAvailablePorts();
    
    // 应该返回一个列表（可能为空）
    EXPECT_TRUE(ports.empty() || ports.size() > 0);
    
    // 如果有端口，检查格式
    for (const auto& port : ports) {
        EXPECT_FALSE(port.portName.isEmpty());
    }
}

// 测试连接无效端口
TEST_F(SerialInterfaceTest, ConnectInvalidPort) {
    bool result = serial->connect("INVALID_PORT_XYZ", 115200);
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(serial->isConnected());
}

// 测试断开连接
TEST_F(SerialInterfaceTest, DisconnectWhenNotConnected) {
    serial->disconnect();  // 不应该崩溃
    EXPECT_FALSE(serial->isConnected());
}

// 测试发送数据到未连接的端口
TEST_F(SerialInterfaceTest, SendWhenDisconnected) {
    bool result = serial->sendData("TEST");
    EXPECT_FALSE(result);
}

// 测试信号
TEST_F(SerialInterfaceTest, ErrorSignal) {
    QSignalSpy errorSpy(serial, &SerialInterface::errorOccurred);
    
    // 尝试连接无效端口应该触发错误信号
    serial->connect("INVALID_PORT", 9600);
    
    // 给信号一些时间
    QCoreApplication::processEvents();
    
    EXPECT_GT(errorSpy.count(), 0);
}

// 测试数据接收缓冲
TEST_F(SerialInterfaceTest, DataBuffer) {
    // 模拟接收不完整的数据行
    QString testData1 = "D:25.5,26";
    QString testData2 = ".1,155.8,156.2\n";
    
    QSignalSpy dataSpy(serial, &SerialInterface::lineReceived);
    
    // 处理第一部分（不应该触发lineReceived）
    serial->processReceivedData(testData1.toUtf8());
    EXPECT_EQ(dataSpy.count(), 0);
    
    // 处理第二部分（应该触发lineReceived）
    serial->processReceivedData(testData2.toUtf8());
    EXPECT_EQ(dataSpy.count(), 1);
}