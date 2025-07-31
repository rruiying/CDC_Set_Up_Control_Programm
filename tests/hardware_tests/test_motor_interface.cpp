#include <gtest/gtest.h>
#include "hardware/include/motor_interface.h"
#include "hardware/include/serial_interface.h"

// Mock串口接口
class MockSerialInterface : public SerialInterface {
public:
    MOCK_METHOD(bool, sendCommand, (const QString&), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
};

class MotorInterfaceTest : public ::testing::Test {
protected:
    MotorInterface* motor;
    MockSerialInterface* mockSerial;
    
    void SetUp() override {
        mockSerial = new MockSerialInterface();
        motor = new MotorInterface(mockSerial);
    }
    
    void TearDown() override {
        delete motor;
        // mockSerial由MotorInterface管理
    }
};

// 测试设置高度命令
TEST_F(MotorInterfaceTest, SetHeightCommand) {
    EXPECT_CALL(*mockSerial, isConnected())
        .WillOnce(Return(true));
    EXPECT_CALL(*mockSerial, sendCommand("H:50.0"))
        .WillOnce(Return(true));
    
    bool result = motor->setHeight(50.0f);
    EXPECT_TRUE(result);
}

// 测试设置角度命令
TEST_F(MotorInterfaceTest, SetAngleCommand) {
    EXPECT_CALL(*mockSerial, isConnected())
        .WillOnce(Return(true));
    EXPECT_CALL(*mockSerial, sendCommand("A:15.5"))
        .WillOnce(Return(true));
    
    bool result = motor->setAngle(15.5f);
    EXPECT_TRUE(result);
}

// 测试停止命令
TEST_F(MotorInterfaceTest, StopCommand) {
    EXPECT_CALL(*mockSerial, isConnected())
        .WillOnce(Return(true));
    EXPECT_CALL(*mockSerial, sendCommand("STOP"))
        .WillOnce(Return(true));
    
    bool result = motor->stop();
    EXPECT_TRUE(result);
}

// 测试归位命令
TEST_F(MotorInterfaceTest, HomeCommand) {
    EXPECT_CALL(*mockSerial, isConnected())
        .WillOnce(Return(true));
    EXPECT_CALL(*mockSerial, sendCommand("HOME"))
        .WillOnce(Return(true));
    
    bool result = motor->home();
    EXPECT_TRUE(result);
}

// 测试未连接时发送命令
TEST_F(MotorInterfaceTest, CommandWhenDisconnected) {
    EXPECT_CALL(*mockSerial, isConnected())
        .WillOnce(Return(false));
    
    bool result = motor->setHeight(50.0f);
    EXPECT_FALSE(result);
}

// 测试设置速度
TEST_F(MotorInterfaceTest, SetSpeed) {
    EXPECT_CALL(*mockSerial, isConnected())
        .WillOnce(Return(true));
    EXPECT_CALL(*mockSerial, sendCommand("S:25.5"))
        .WillOnce(Return(true));
    
    bool result = motor->setSpeed(25.5f);
    EXPECT_TRUE(result);
}