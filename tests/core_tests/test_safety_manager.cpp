// tests/core_tests/test_safety_manager.cpp
#include <gtest/gtest.h>
#include "core/include/safety_manager.h"
#include "models/include/system_config.h"
#include "utils/include/logger.h"

class SafetyManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setConsoleOutput(false);
        SystemConfig::getInstance().reset();
        
        safetyManager = std::make_unique<SafetyManager>();
    }
    
    std::unique_ptr<SafetyManager> safetyManager;
};

// 测试基本位置检查
TEST_F(SafetyManagerTest, BasicPositionCheck) {
    // 使用默认限位 (0-100mm, -90-90度)
    
    // 有效位置
    EXPECT_TRUE(safetyManager->checkPosition(50.0, 0.0));
    EXPECT_TRUE(safetyManager->checkPosition(0.0, -90.0));
    EXPECT_TRUE(safetyManager->checkPosition(100.0, 90.0));
    
    // 无效位置
    EXPECT_FALSE(safetyManager->checkPosition(-10.0, 0.0));
    EXPECT_FALSE(safetyManager->checkPosition(150.0, 0.0));
    EXPECT_FALSE(safetyManager->checkPosition(50.0, -100.0));
    EXPECT_FALSE(safetyManager->checkPosition(50.0, 100.0));
}

// 测试自定义限位
TEST_F(SafetyManagerTest, CustomLimits) {
    // 设置自定义限位
    SystemConfig::getInstance().setSafetyLimits(10.0, 90.0, -45.0, 45.0);
    safetyManager->updateLimitsFromConfig();
    
    // 边界测试
    EXPECT_TRUE(safetyManager->checkPosition(10.0, -45.0));
    EXPECT_TRUE(safetyManager->checkPosition(90.0, 45.0));
    EXPECT_TRUE(safetyManager->checkPosition(50.0, 0.0));
    
    // 超出边界
    EXPECT_FALSE(safetyManager->checkPosition(5.0, 0.0));
    EXPECT_FALSE(safetyManager->checkPosition(95.0, 0.0));
    EXPECT_FALSE(safetyManager->checkPosition(50.0, -50.0));
    EXPECT_FALSE(safetyManager->checkPosition(50.0, 50.0));
}

// 测试移动路径检查
TEST_F(SafetyManagerTest, MovementPathCheck) {
    // 设置当前位置
    safetyManager->setCurrentPosition(50.0, 0.0);
    
    // 正常移动
    EXPECT_TRUE(safetyManager->checkMovement(50.0, 0.0, 60.0, 10.0));
    
    // 移动距离过大（假设最大单次移动50mm）
    safetyManager->setMaxSingleMove(50.0, 45.0);
    EXPECT_FALSE(safetyManager->checkMovement(10.0, 0.0, 70.0, 0.0)); // 60mm移动
    EXPECT_TRUE(safetyManager->checkMovement(10.0, 0.0, 50.0, 0.0));  // 40mm移动
    
    // 角度变化过大
    EXPECT_FALSE(safetyManager->checkMovement(50.0, 0.0, 50.0, 50.0)); // 50度变化
    EXPECT_TRUE(safetyManager->checkMovement(50.0, 0.0, 50.0, 40.0));  // 40度变化
}

// 测试速度限制
TEST_F(SafetyManagerTest, SpeedLimitCheck) {
    // 设置速度限制
    safetyManager->setSpeedLimits(10.0, 5.0); // 10mm/s, 5deg/s
    
    // 检查移动时间是否足够
    double time = safetyManager->calculateMinimumMoveTime(0.0, 0.0, 50.0, 20.0);
    EXPECT_GE(time, 5.0); // 至少需要5秒（50mm / 10mm/s）
    
    // 验证速度检查
    EXPECT_TRUE(safetyManager->checkMoveSpeed(0.0, 0.0, 10.0, 5.0, 2.0));   // 5mm/s, 2.5deg/s - OK
    EXPECT_FALSE(safetyManager->checkMoveSpeed(0.0, 0.0, 30.0, 15.0, 1.0)); // 30mm/s, 15deg/s - 太快
}

// 测试紧急停止
TEST_F(SafetyManagerTest, EmergencyStop) {
    EXPECT_FALSE(safetyManager->isEmergencyStopped());
    
    // 触发紧急停止
    safetyManager->triggerEmergencyStop("Test emergency");
    EXPECT_TRUE(safetyManager->isEmergencyStopped());
    EXPECT_FALSE(safetyManager->checkPosition(50.0, 0.0)); // 任何位置都不安全
    
    // 获取紧急停止原因
    EXPECT_EQ(safetyManager->getEmergencyStopReason(), "Test emergency");
    
    // 清除紧急停止
    safetyManager->clearEmergencyStop();
    EXPECT_FALSE(safetyManager->isEmergencyStopped());
    EXPECT_TRUE(safetyManager->checkPosition(50.0, 0.0)); // 恢复正常
}

// 测试碰撞检测区域
TEST_F(SafetyManagerTest, CollisionZones) {
    // 添加禁止区域
    safetyManager->addForbiddenZone(40.0, 60.0, -10.0, 10.0);
    
    // 测试区域内外的位置
    EXPECT_FALSE(safetyManager->checkPosition(50.0, 0.0));  // 在禁止区域内
    EXPECT_TRUE(safetyManager->checkPosition(30.0, 0.0));   // 在禁止区域外
    EXPECT_TRUE(safetyManager->checkPosition(70.0, 0.0));   // 在禁止区域外
    EXPECT_TRUE(safetyManager->checkPosition(50.0, 15.0));  // 在禁止区域外
    
    // 清除禁止区域
    safetyManager->clearForbiddenZones();
    EXPECT_TRUE(safetyManager->checkPosition(50.0, 0.0));   // 现在OK了
}

// 测试安全模式
TEST_F(SafetyManagerTest, SafetyModes) {
    // 正常模式
    safetyManager->setSafetyMode(SafetyMode::NORMAL);
    EXPECT_TRUE(safetyManager->checkPosition(90.0, 80.0));
    
    // 限制模式（减少限位范围）
    safetyManager->setSafetyMode(SafetyMode::RESTRICTED);
    EXPECT_FALSE(safetyManager->checkPosition(90.0, 80.0)); // 在限制模式下超出范围
    EXPECT_TRUE(safetyManager->checkPosition(50.0, 40.0));  // 在减少的范围内
    
    // 维护模式（扩大限位范围）
    safetyManager->setSafetyMode(SafetyMode::MAINTENANCE);
    EXPECT_TRUE(safetyManager->checkPosition(105.0, 95.0)); // 允许稍微超出正常范围
}

// 测试安全违规记录
TEST_F(SafetyManagerTest, ViolationLogging) {
    // 初始状态
    EXPECT_EQ(safetyManager->getViolationCount(), 0);
    
    // 触发一些违规
    safetyManager->checkPosition(-10.0, 0.0);  // 高度太低
    safetyManager->checkPosition(150.0, 0.0); // 高度太高
    safetyManager->checkPosition(50.0, 100.0); // 角度太大
    
    // 检查违规计数
    EXPECT_EQ(safetyManager->getViolationCount(), 3);
    
    // 获取违规历史
    auto violations = safetyManager->getViolationHistory();
    EXPECT_EQ(violations.size(), 3);
    
    // 清除违规记录
    safetyManager->clearViolationHistory();
    EXPECT_EQ(safetyManager->getViolationCount(), 0);
}

// 测试安全回调
TEST_F(SafetyManagerTest, SafetyCallbacks) {
    bool violationDetected = false;
    std::string violationReason;
    
    // 设置违规回调
    safetyManager->setViolationCallback([&](const std::string& reason) {
        violationDetected = true;
        violationReason = reason;
    });
    
    // 触发违规
    safetyManager->checkPosition(-10.0, 0.0);
    
    EXPECT_TRUE(violationDetected);
    EXPECT_FALSE(violationReason.empty());
    EXPECT_TRUE(violationReason.find("height") != std::string::npos);
}

// 测试配置同步
TEST_F(SafetyManagerTest, ConfigSync) {
    // 初始限位
    EXPECT_TRUE(safetyManager->checkPosition(95.0, 85.0));
    
    // 更改系统配置
    SystemConfig::getInstance().setSafetyLimits(10.0, 80.0, -45.0, 45.0);
    
    // 手动同步
    safetyManager->updateLimitsFromConfig();
    
    // 验证新限位生效
    EXPECT_FALSE(safetyManager->checkPosition(95.0, 85.0));
    EXPECT_TRUE(safetyManager->checkPosition(50.0, 30.0));
}

// 测试多线程安全
TEST_F(SafetyManagerTest, ThreadSafety) {
    const int numThreads = 10;
    const int checksPerThread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> passCount{0};
    
    // 启动多个线程进行检查
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([this, &passCount, i]() {
            for (int j = 0; j < checksPerThread; j++) {
                double height = (i * 10.0) + (j * 0.1);
                double angle = (i * 5.0) - 45.0;
                
                if (safetyManager->checkPosition(height, angle)) {
                    passCount++;
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 验证结果一致性
    int expectedPass = 0;
    for (int i = 0; i < numThreads; i++) {
        for (int j = 0; j < checksPerThread; j++) {
            double height = (i * 10.0) + (j * 0.1);
            double angle = (i * 5.0) - 45.0;
            
            if (SystemConfig::getInstance().isPositionValid(height, angle)) {
                expectedPass++;
            }
        }
    }
    
    EXPECT_EQ(passCount.load(), expectedPass);
}