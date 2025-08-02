// tests/core_tests/test_motor_controller.cpp
#include <gtest/gtest.h>
#include "core/include/motor_controller.h"
#include "core/include/safety_manager.h"
#include "hardware/include/serial_interface.h"
#include "models/include/system_config.h"
#include "utils/include/logger.h"
#include <thread>
#include <chrono>

// 模拟串口用于测试
class MockSerialForMotor : public SerialInterface {
public:
    MockSerialForMotor() {
        setMockMode(true);
    }
    
    void simulateOkResponse() {
        mockResponses.push_back("OK\r\n");
    }
    
    void simulateStatusResponse(const std::string& status, double height, double angle) {
        std::ostringstream oss;
        oss << "STATUS:" << status << "," << height << "," << angle << "\r\n";
        mockResponses.push_back(oss.str());
    }
    
    void simulateErrorResponse(const std::string& error) {
        mockResponses.push_back("ERROR:" + error + "\r\n");
    }
    
    std::vector<std::string> getSentCommands() const {
        return sentCommands;
    }
    
    std::queue<std::string> mockResponses;
    std::vector<std::string> sentCommands;
};

class MotorControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setConsoleOutput(false);
        SystemConfig::getInstance().reset();
        
        mockSerial = std::make_shared<MockSerialForMotor>();
        safetyManager = std::make_shared<SafetyManager>();
        motorController = std::make_unique<MotorController>(mockSerial, safetyManager);
    }
    
    std::shared_ptr<MockSerialForMotor> mockSerial;
    std::shared_ptr<SafetyManager> safetyManager;
    std::unique_ptr<MotorController> motorController;
};

// 测试初始化
TEST_F(MotorControllerTest, Initialization) {
    EXPECT_EQ(motorController->getStatus(), MotorStatus::IDLE);
    EXPECT_FALSE(motorController->isMoving());
    EXPECT_DOUBLE_EQ(motorController->getCurrentHeight(), 0.0);
    EXPECT_DOUBLE_EQ(motorController->getCurrentAngle(), 0.0);
}

// 测试设置高度
TEST_F(MotorControllerTest, SetHeight) {
    mockSerial->simulateOkResponse();
    
    bool success = motorController->setHeight(25.0);
    EXPECT_TRUE(success);
    
    // 验证发送的命令
    auto commands = mockSerial->getSentCommands();
    EXPECT_EQ(commands.size(), 1);
    EXPECT_EQ(commands[0], "SET_HEIGHT:25\r\n");
}

// 测试设置角度
TEST_F(MotorControllerTest, SetAngle) {
    mockSerial->simulateOkResponse();
    
    bool success = motorController->setAngle(5.5);
    EXPECT_TRUE(success);
    
    auto commands = mockSerial->getSentCommands();
    EXPECT_EQ(commands.size(), 1);
    EXPECT_EQ(commands[0], "SET_ANGLE:5.5\r\n");
}

// 测试移动到位置
TEST_F(MotorControllerTest, MoveToPosition) {
    // 准备响应
    mockSerial->simulateOkResponse(); // 对MOVE_TO命令的响应
    mockSerial->simulateStatusResponse("MOVING", 12.5, 2.5);
    mockSerial->simulateStatusResponse("READY", 25.0, 5.0);
    
    // 设置状态更新回调
    bool movingDetected = false;
    bool completedDetected = false;
    
    motorController->setStatusCallback([&](MotorStatus status) {
        if (status == MotorStatus::MOVING) movingDetected = true;
        if (status == MotorStatus::IDLE) completedDetected = true;
    });
    
    // 执行移动
    bool success = motorController->moveToPosition(25.0, 5.0);
    EXPECT_TRUE(success);
    
    // 等待移动完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(movingDetected);
    EXPECT_TRUE(completedDetected);
    EXPECT_DOUBLE_EQ(motorController->getCurrentHeight(), 25.0);
    EXPECT_DOUBLE_EQ(motorController->getCurrentAngle(), 5.0);
}

// 测试停止
TEST_F(MotorControllerTest, Stop) {
    mockSerial->simulateOkResponse();
    
    bool success = motorController->stop();
    EXPECT_TRUE(success);
    
    auto commands = mockSerial->getSentCommands();
    EXPECT_EQ(commands.size(), 1);
    EXPECT_EQ(commands[0], "STOP\r\n");
}

// 测试紧急停止
TEST_F(MotorControllerTest, EmergencyStop) {
    mockSerial->simulateOkResponse();
    
    bool success = motorController->emergencyStop();
    EXPECT_TRUE(success);
    EXPECT_EQ(motorController->getStatus(), MotorStatus::ERROR);
    
    auto commands = mockSerial->getSentCommands();
    EXPECT_EQ(commands.size(), 1);
    EXPECT_EQ(commands[0], "EMERGENCY_STOP\r\n");
}

// 测试回零
TEST_F(MotorControllerTest, Home) {
    // 准备响应
    mockSerial->simulateOkResponse();
    mockSerial->simulateStatusResponse("MOVING", 25.0, 5.0);
    mockSerial->simulateStatusResponse("READY", 50.0, 0.0);
    
    bool success = motorController->home();
    EXPECT_TRUE(success);
    
    // 等待回零完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_DOUBLE_EQ(motorController->getCurrentHeight(), 50.0);
    EXPECT_DOUBLE_EQ(motorController->getCurrentAngle(), 0.0);
}

// 测试安全限位检查
TEST_F(MotorControllerTest, SafetyLimits) {
    // 设置安全限位
    SystemConfig::getInstance().setSafetyLimits(10.0, 90.0, -30.0, 30.0);
    
    // 超出高度范围
    bool success = motorController->setHeight(100.0);
    EXPECT_FALSE(success);
    
    // 超出角度范围
    success = motorController->setAngle(45.0);
    EXPECT_FALSE(success);
    
    // 在范围内
    mockSerial->simulateOkResponse();
    success = motorController->setHeight(50.0);
    EXPECT_TRUE(success);
}

// 测试错误处理
TEST_F(MotorControllerTest, ErrorHandling) {
    // 模拟错误响应
    mockSerial->simulateErrorResponse("OUT_OF_RANGE");
    
    bool success = motorController->setHeight(25.0);
    EXPECT_FALSE(success);
    
    // 验证错误信息
    auto errorInfo = motorController->getLastError();
    EXPECT_FALSE(errorInfo.message.empty());
    EXPECT_TRUE(errorInfo.message.find("OUT_OF_RANGE") != std::string::npos);
}

// 测试超时处理
TEST_F(MotorControllerTest, TimeoutHandling) {
    // 设置短超时
    motorController->setCommandTimeout(100);
    
    // 不提供响应，导致超时
    bool success = motorController->setHeight(25.0);
    EXPECT_FALSE(success);
    
    auto errorInfo = motorController->getLastError();
    EXPECT_TRUE(errorInfo.message.find("Timeout") != std::string::npos ||
                errorInfo.message.find("timeout") != std::string::npos);
}

// 测试速度控制
TEST_F(MotorControllerTest, SpeedControl) {
    // 设置速度
    motorController->setSpeed(MotorSpeed::SLOW);
    EXPECT_EQ(motorController->getSpeed(), MotorSpeed::SLOW);
    
    motorController->setSpeed(MotorSpeed::FAST);
    EXPECT_EQ(motorController->getSpeed(), MotorSpeed::FAST);
    
    // 速度应该保存到系统配置
    EXPECT_EQ(SystemConfig::getInstance().getMotorSpeed(), MotorSpeed::FAST);
}

// 测试位置跟踪
TEST_F(MotorControllerTest, PositionTracking) {
    // 模拟一系列移动
    mockSerial->simulateOkResponse();
    motorController->setHeight(25.0);
    motorController->updateCurrentPosition(25.0, 0.0);
    
    mockSerial->simulateOkResponse();
    motorController->setAngle(5.0);
    motorController->updateCurrentPosition(25.0, 5.0);
    
    // 验证位置历史
    EXPECT_DOUBLE_EQ(motorController->getCurrentHeight(), 25.0);
    EXPECT_DOUBLE_EQ(motorController->getCurrentAngle(), 5.0);
    EXPECT_DOUBLE_EQ(motorController->getTargetHeight(), 25.0);
    EXPECT_DOUBLE_EQ(motorController->getTargetAngle(), 5.0);
}

// 测试状态查询
TEST_F(MotorControllerTest, StatusQuery) {
    mockSerial->simulateStatusResponse("READY", 30.0, 10.0);
    
    bool success = motorController->updateStatus();
    EXPECT_TRUE(success);
    
    EXPECT_EQ(motorController->getStatus(), MotorStatus::IDLE);
    EXPECT_DOUBLE_EQ(motorController->getCurrentHeight(), 30.0);
    EXPECT_DOUBLE_EQ(motorController->getCurrentAngle(), 10.0);
}

// 测试异步移动
TEST_F(MotorControllerTest, AsyncMove) {
    // 准备响应序列
    mockSerial->simulateOkResponse();
    for (int i = 0; i < 5; i++) {
        mockSerial->simulateStatusResponse("MOVING", 20.0 + i * 2, i * 0.5);
    }
    mockSerial->simulateStatusResponse("READY", 30.0, 2.5);
    
    // 启动异步移动
    motorController->moveToPositionAsync(30.0, 2.5);
    
    // 等待移动开始
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(motorController->isMoving());
    
    // 等待移动完成
    motorController->waitForCompletion(1000);
    EXPECT_FALSE(motorController->isMoving());
    EXPECT_DOUBLE_EQ(motorController->getCurrentHeight(), 30.0);
    EXPECT_DOUBLE_EQ(motorController->getCurrentAngle(), 2.5);
}

// 测试移动中断
TEST_F(MotorControllerTest, InterruptMove) {
    // 开始移动
    mockSerial->simulateOkResponse();
    motorController->moveToPositionAsync(50.0, 10.0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // 中断移动
    mockSerial->simulateOkResponse();
    bool success = motorController->stop();
    EXPECT_TRUE(success);
    EXPECT_FALSE(motorController->isMoving());
}

// 测试错误恢复
TEST_F(MotorControllerTest, ErrorRecovery) {
    // 触发错误
    mockSerial->simulateErrorResponse("HARDWARE_ERROR");
    motorController->setHeight(25.0);
    EXPECT_EQ(motorController->getStatus(), MotorStatus::ERROR);
    
    // 清除错误
    motorController->clearError();
    EXPECT_EQ(motorController->getStatus(), MotorStatus::IDLE);
    
    // 应该可以再次操作
    mockSerial->simulateOkResponse();
    bool success = motorController->setHeight(30.0);
    EXPECT_TRUE(success);
}

// 测试批量命令
TEST_F(MotorControllerTest, BatchCommands) {
    // 准备响应
    for (int i = 0; i < 3; i++) {
        mockSerial->simulateOkResponse();
    }
    
    // 执行批量命令
    std::vector<MotorCommand> commands = {
        {MotorCommandType::SET_HEIGHT, 25.0, 0.0},
        {MotorCommandType::SET_ANGLE, 0.0, 5.0},
        {MotorCommandType::MOVE_TO, 25.0, 5.0}
    };
    
    bool success = motorController->executeBatch(commands);
    EXPECT_TRUE(success);
    
    // 验证所有命令都被发送
    auto sentCommands = mockSerial->getSentCommands();
    EXPECT_EQ(sentCommands.size(), 3);
}

// 测试移动进度回调
TEST_F(MotorControllerTest, ProgressCallback) {
    std::vector<double> progressValues;
    
    motorController->setProgressCallback([&progressValues](double progress) {
        progressValues.push_back(progress);
    });
    
    // 模拟移动过程
    mockSerial->simulateOkResponse();
    mockSerial->simulateStatusResponse("MOVING", 10.0, 0.0);
    mockSerial->simulateStatusResponse("MOVING", 15.0, 2.5);
    mockSerial->simulateStatusResponse("MOVING", 20.0, 5.0);
    mockSerial->simulateStatusResponse("READY", 20.0, 5.0);
    
    motorController->moveToPosition(20.0, 5.0);
    
    // 等待完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 应该收到多次进度更新
    EXPECT_GT(progressValues.size(), 0);
    EXPECT_DOUBLE_EQ(progressValues.back(), 100.0); // 最后应该是100%
}