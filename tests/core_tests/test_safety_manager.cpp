#include <gtest/gtest.h>
#include "core/include/safety_manager.h"

class SafetyManagerTest : public ::testing::Test {
protected:
    SafetyManager* safety;
    
    void SetUp() override {
        safety = new SafetyManager();
    }
    
    void TearDown() override {
        delete safety;
    }
};

// 测试高度限制
TEST_F(SafetyManagerTest, HeightLimits) {
    safety->setHeightLimits(0.0f, 100.0f);
    
    EXPECT_TRUE(safety->isHeightSafe(50.0f));
    EXPECT_TRUE(safety->isHeightSafe(0.0f));
    EXPECT_TRUE(safety->isHeightSafe(100.0f));
    
    EXPECT_FALSE(safety->isHeightSafe(-10.0f));
    EXPECT_FALSE(safety->isHeightSafe(150.0f));
}

// 测试角度限制
TEST_F(SafetyManagerTest, AngleLimits) {
    safety->setAngleLimits(-45.0f, 45.0f);
    
    EXPECT_TRUE(safety->isAngleSafe(0.0f));
    EXPECT_TRUE(safety->isAngleSafe(45.0f));
    EXPECT_TRUE(safety->isAngleSafe(-45.0f));
    
    EXPECT_FALSE(safety->isAngleSafe(50.0f));
    EXPECT_FALSE(safety->isAngleSafe(-50.0f));
}

// 测试速度模式
TEST_F(SafetyManagerTest, SpeedModes) {
    safety->setSpeedMode(SafetyManager::SpeedMode::Slow);
    EXPECT_EQ(safety->getMaxSpeed(), 10.0f);
    
    safety->setSpeedMode(SafetyManager::SpeedMode::Medium);
    EXPECT_EQ(safety->getMaxSpeed(), 30.0f);
    
    safety->setSpeedMode(SafetyManager::SpeedMode::Fast);
    EXPECT_EQ(safety->getMaxSpeed(), 50.0f);
}

// 测试紧急停止
TEST_F(SafetyManagerTest, EmergencyStop) {
    EXPECT_FALSE(safety->isEmergencyStopped());
    
    safety->triggerEmergencyStop();
    EXPECT_TRUE(safety->isEmergencyStopped());
    
    // 紧急停止后，所有运动都应该被禁止
    EXPECT_FALSE(safety->isMovementAllowed());
    
    safety->resetEmergencyStop();
    EXPECT_FALSE(safety->isEmergencyStopped());
    EXPECT_TRUE(safety->isMovementAllowed());
}

// 测试传感器数据有效性检查
TEST_F(SafetyManagerTest, SensorDataValidation) {
    SensorData data;
    data.distanceUpper1 = 25.0f;
    data.distanceUpper2 = 26.0f;
    data.temperature = 23.5f;
    
    EXPECT_TRUE(safety->isSensorDataValid(data));
    
    // 测试异常值
    data.temperature = 100.0f;  // 太高
    EXPECT_FALSE(safety->isSensorDataValid(data));
    
    data.temperature = 25.0f;
    data.distanceUpper1 = -10.0f;  // 负值
    EXPECT_FALSE(safety->isSensorDataValid(data));
}

// 测试运动请求验证
TEST_F(SafetyManagerTest, ValidateMovementRequest) {
    safety->setHeightLimits(0.0f, 100.0f);
    safety->setAngleLimits(-45.0f, 45.0f);
    
    SafetyManager::MovementRequest request;
    request.targetHeight = 50.0f;
    request.targetAngle = 20.0f;
    request.speed = 20.0f;
    
    EXPECT_TRUE(safety->validateMovementRequest(request));
    
    // 超出限制
    request.targetHeight = 150.0f;
    EXPECT_FALSE(safety->validateMovementRequest(request));
}