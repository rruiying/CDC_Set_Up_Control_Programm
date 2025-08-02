// tests/models_tests/test_system_config.cpp
#include <gtest/gtest.h>
#include "models/include/system_config.h"
#include <filesystem>
#include <fstream>

class SystemConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试配置文件
        testConfigFile = "test_config.json";
        
        // 重置单例
        SystemConfig::getInstance().reset();
    }
    
    void TearDown() override {
        // 清理测试文件
        if (std::filesystem::exists(testConfigFile)) {
            std::filesystem::remove(testConfigFile);
        }
    }
    
    void createTestConfigFile() {
        std::ofstream file(testConfigFile);
        file << R"({
            "safetyLimits": {
                "minHeight": 5.0,
                "maxHeight": 95.0,
                "minAngle": -45.0,
                "maxAngle": 45.0
            },
            "capacitorPlate": {
                "area": 0.01,
                "dielectricConstant": 1.0
            },
            "systemDimensions": {
                "totalHeight": 200.0,
                "middlePlateHeight": 25.0,
                "sensorSpacing": 100.0
            },
            "motorControl": {
                "defaultSpeed": "Medium",
                "homePosition": {
                    "height": 50.0,
                    "angle": 0.0
                }
            },
            "communication": {
                "defaultBaudRate": 115200,
                "timeout": 5000,
                "retryCount": 3
            },
            "dataRecording": {
                "sensorUpdateInterval": 2000,
                "maxRecords": 10000,
                "autoSaveInterval": 300000
            }
        })";
        file.close();
    }
    
    std::string testConfigFile;
};

// 测试单例模式
TEST_F(SystemConfigTest, SingletonPattern) {
    SystemConfig& config1 = SystemConfig::getInstance();
    SystemConfig& config2 = SystemConfig::getInstance();
    
    EXPECT_EQ(&config1, &config2);
}

// 测试默认值
TEST_F(SystemConfigTest, DefaultValues) {
    SystemConfig& config = SystemConfig::getInstance();
    
    // 安全限位默认值
    EXPECT_DOUBLE_EQ(config.getMinHeight(), 0.0);
    EXPECT_DOUBLE_EQ(config.getMaxHeight(), 100.0);
    EXPECT_DOUBLE_EQ(config.getMinAngle(), -90.0);
    EXPECT_DOUBLE_EQ(config.getMaxAngle(), 90.0);
    
    // 电容板参数默认值
    EXPECT_DOUBLE_EQ(config.getPlateArea(), 0.01);
    EXPECT_DOUBLE_EQ(config.getDielectricConstant(), 1.0);
    
    // 系统尺寸默认值
    EXPECT_DOUBLE_EQ(config.getTotalHeight(), 200.0);
    EXPECT_DOUBLE_EQ(config.getMiddlePlateHeight(), 25.0);
    EXPECT_DOUBLE_EQ(config.getSensorSpacing(), 100.0);
    
    // 通信参数默认值
    EXPECT_EQ(config.getDefaultBaudRate(), 115200);
    EXPECT_EQ(config.getCommunicationTimeout(), 5000);
    
    // 数据记录参数默认值
    EXPECT_EQ(config.getSensorUpdateInterval(), 2000);
}

// 测试从文件加载配置
TEST_F(SystemConfigTest, LoadFromFile) {
    createTestConfigFile();
    
    SystemConfig& config = SystemConfig::getInstance();
    EXPECT_TRUE(config.loadFromFile(testConfigFile));
    
    // 验证加载的值
    EXPECT_DOUBLE_EQ(config.getMinHeight(), 5.0);
    EXPECT_DOUBLE_EQ(config.getMaxHeight(), 95.0);
    EXPECT_DOUBLE_EQ(config.getMinAngle(), -45.0);
    EXPECT_DOUBLE_EQ(config.getMaxAngle(), 45.0);
    
    EXPECT_EQ(config.getMotorSpeed(), MotorSpeed::MEDIUM);
    EXPECT_DOUBLE_EQ(config.getHomeHeight(), 50.0);
    EXPECT_DOUBLE_EQ(config.getHomeAngle(), 0.0);
}

// 测试保存到文件
TEST_F(SystemConfigTest, SaveToFile) {
    SystemConfig& config = SystemConfig::getInstance();
    
    // 修改一些值
    config.setSafetyLimits(10.0, 90.0, -30.0, 30.0);
    config.setPlateArea(0.015);
    config.setMotorSpeed(MotorSpeed::FAST);
    
    // 保存到文件
    EXPECT_TRUE(config.saveToFile(testConfigFile));
    
    // 重置并重新加载
    config.reset();
    EXPECT_TRUE(config.loadFromFile(testConfigFile));
    
    // 验证保存的值
    EXPECT_DOUBLE_EQ(config.getMinHeight(), 10.0);
    EXPECT_DOUBLE_EQ(config.getMaxHeight(), 90.0);
    EXPECT_DOUBLE_EQ(config.getPlateArea(), 0.015);
    EXPECT_EQ(config.getMotorSpeed(), MotorSpeed::FAST);
}

// 测试安全限位验证
TEST_F(SystemConfigTest, SafetyLimitValidation) {
    SystemConfig& config = SystemConfig::getInstance();
    config.setSafetyLimits(10.0, 90.0, -30.0, 30.0);
    
    // 在范围内
    EXPECT_TRUE(config.isHeightInRange(50.0));
    EXPECT_TRUE(config.isAngleInRange(0.0));
    
    // 边界值
    EXPECT_TRUE(config.isHeightInRange(10.0));
    EXPECT_TRUE(config.isHeightInRange(90.0));
    EXPECT_TRUE(config.isAngleInRange(-30.0));
    EXPECT_TRUE(config.isAngleInRange(30.0));
    
    // 超出范围
    EXPECT_FALSE(config.isHeightInRange(5.0));
    EXPECT_FALSE(config.isHeightInRange(95.0));
    EXPECT_FALSE(config.isAngleInRange(-35.0));
    EXPECT_FALSE(config.isAngleInRange(35.0));
}

// 测试位置验证
TEST_F(SystemConfigTest, PositionValidation) {
    SystemConfig& config = SystemConfig::getInstance();
    config.setSafetyLimits(10.0, 90.0, -30.0, 30.0);
    
    EXPECT_TRUE(config.isPositionValid(50.0, 0.0));
    EXPECT_TRUE(config.isPositionValid(10.0, -30.0));
    EXPECT_FALSE(config.isPositionValid(5.0, 0.0));
    EXPECT_FALSE(config.isPositionValid(50.0, 40.0));
    EXPECT_FALSE(config.isPositionValid(100.0, 50.0));
}

// 测试电机速度转换
TEST_F(SystemConfigTest, MotorSpeedConversion) {
    SystemConfig& config = SystemConfig::getInstance();
    
    config.setMotorSpeed(MotorSpeed::SLOW);
    EXPECT_EQ(config.getMotorSpeed(), MotorSpeed::SLOW);
    EXPECT_EQ(config.getMotorSpeedString(), "Slow");
    
    config.setMotorSpeed(MotorSpeed::MEDIUM);
    EXPECT_EQ(config.getMotorSpeedString(), "Medium");
    
    config.setMotorSpeed(MotorSpeed::FAST);
    EXPECT_EQ(config.getMotorSpeedString(), "Fast");
    
    // 从字符串设置
    config.setMotorSpeedFromString("Slow");
    EXPECT_EQ(config.getMotorSpeed(), MotorSpeed::SLOW);
    
    config.setMotorSpeedFromString("Medium");
    EXPECT_EQ(config.getMotorSpeed(), MotorSpeed::MEDIUM);
    
    config.setMotorSpeedFromString("Fast");
    EXPECT_EQ(config.getMotorSpeed(), MotorSpeed::FAST);
    
    // 无效字符串应该保持不变
    MotorSpeed original = config.getMotorSpeed();
    config.setMotorSpeedFromString("Invalid");
    EXPECT_EQ(config.getMotorSpeed(), original);
}

// 测试配置修改通知
TEST_F(SystemConfigTest, ConfigChangeNotification) {
    SystemConfig& config = SystemConfig::getInstance();
    
    bool notified = false;
    config.setConfigChangeCallback([&notified]() {
        notified = true;
    });
    
    // 修改配置应该触发回调
    config.setSafetyLimits(20.0, 80.0, -20.0, 20.0);
    EXPECT_TRUE(notified);
    
    notified = false;
    config.setPlateArea(0.02);
    EXPECT_TRUE(notified);
}

// 测试配置验证
TEST_F(SystemConfigTest, ConfigValidation) {
    SystemConfig& config = SystemConfig::getInstance();
    
    // 有效的安全限位
    EXPECT_TRUE(config.setSafetyLimits(10.0, 90.0, -45.0, 45.0));
    
    // 无效的安全限位（最小值大于最大值）
    EXPECT_FALSE(config.setSafetyLimits(90.0, 10.0, -45.0, 45.0));
    EXPECT_FALSE(config.setSafetyLimits(10.0, 90.0, 45.0, -45.0));
    
    // 无效的板面积
    EXPECT_FALSE(config.setPlateArea(-0.01));
    EXPECT_FALSE(config.setPlateArea(0.0));
    
    // 无效的介电常数
    EXPECT_FALSE(config.setDielectricConstant(-1.0));
    EXPECT_FALSE(config.setDielectricConstant(0.0));
}

// 测试获取所有支持的波特率
TEST_F(SystemConfigTest, SupportedBaudRates) {
    SystemConfig& config = SystemConfig::getInstance();
    
    auto baudRates = config.getSupportedBaudRates();
    
    // 应该包含标准波特率
    EXPECT_TRUE(std::find(baudRates.begin(), baudRates.end(), 9600) != baudRates.end());
    EXPECT_TRUE(std::find(baudRates.begin(), baudRates.end(), 19200) != baudRates.end());
    EXPECT_TRUE(std::find(baudRates.begin(), baudRates.end(), 38400) != baudRates.end());
    EXPECT_TRUE(std::find(baudRates.begin(), baudRates.end(), 57600) != baudRates.end());
    EXPECT_TRUE(std::find(baudRates.begin(), baudRates.end(), 115200) != baudRates.end());
}

// 测试配置重置
TEST_F(SystemConfigTest, ConfigReset) {
    SystemConfig& config = SystemConfig::getInstance();
    
    // 修改一些值
    config.setSafetyLimits(20.0, 80.0, -20.0, 20.0);
    config.setPlateArea(0.02);
    
    // 重置到默认值
    config.reset();
    
    // 验证恢复到默认值
    EXPECT_DOUBLE_EQ(config.getMinHeight(), 0.0);
    EXPECT_DOUBLE_EQ(config.getMaxHeight(), 100.0);
    EXPECT_DOUBLE_EQ(config.getPlateArea(), 0.01);
}

// 测试无效文件处理
TEST_F(SystemConfigTest, InvalidFileHandling) {
    SystemConfig& config = SystemConfig::getInstance();
    
    // 尝试加载不存在的文件
    EXPECT_FALSE(config.loadFromFile("non_existent_file.json"));
    
    // 创建无效的JSON文件
    std::ofstream invalidFile("invalid.json");
    invalidFile << "{ invalid json content";
    invalidFile.close();
    
    EXPECT_FALSE(config.loadFromFile("invalid.json"));
    
    // 清理
    std::filesystem::remove("invalid.json");
}