#include <gtest/gtest.h>
#include <QSignalSpy>
#include "core/include/motor_controller.h"
#include "core/include/safety_manager.h"
#include "hardware/include/motor_interface.h"

class MockMotorInterface : public MotorInterface {
public:
    MockMotorInterface() : MotorInterface(nullptr) {}
    
    MOCK_METHOD(bool, setHeight, (float), (override));
    MOCK_METHOD(bool, setAngle, (float), (override));
    MOCK_METHOD(bool, stop, (), (override));
    MOCK_METHOD(bool, home, (), (override));
};

class MotorControllerTest : public ::testing::Test {
protected:
    MotorController* controller;
    MockMotorInterface* mockMotor;
    SafetyManager* safety;
    
    void SetUp() override {
        mockMotor = new MockMotorInterface();
        safety = new SafetyManager();
        controller = new MotorController(mockMotor, safety);
    }
    
    void TearDown() override {
        delete controller;
    }
};

// 测试移动到位置
TEST_F(MotorControllerTest, MoveToPosition) {
    safety->setHeightLimits(0, 100);
    safety->setAngleLimits(-45, 45);
    
    EXPECT_CALL(*mockMotor, setHeight(50.0f))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockMotor, setAngle(20.0f))
        .WillOnce(Return(true));
    
    bool result = controller->moveToPosition(50.0f, 20.0f);
    EXPECT_TRUE(result);
}

// 测试超出限制的移动
TEST_F(MotorControllerTest, MoveOutOfLimits) {
    safety->setHeightLimits(0, 100);
    
    // 不应该调用motor接口
    EXPECT_CALL(*mockMotor, setHeight(testing::_))
        .Times(0);
    
    bool result = controller->moveToPosition(150.0f, 0.0f);
    EXPECT_FALSE(result);
}

// 测试紧急停止
TEST_F(MotorControllerTest, EmergencyStop) {
    EXPECT_CALL(*mockMotor, stop())
        .WillOnce(Return(true));
    
    controller->emergencyStop();
    EXPECT_TRUE(controller->isStopped());
}

// 测试归位
TEST_F(MotorControllerTest, HomePosition) {
    EXPECT_CALL(*mockMotor, home())
        .WillOnce(Return(true));
    
    bool result = controller->goHome();
    EXPECT_TRUE(result);
}

// 测试状态更新
TEST_F(MotorControllerTest, StatusUpdate) {
    QSignalSpy statusSpy(controller, &MotorController::statusChanged);
    
    controller->setTargetHeight(50.0f);
    
    // 应该触发状态更新
    EXPECT_GT(statusSpy.count(), 0);
}