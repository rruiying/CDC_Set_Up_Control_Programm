#include <gtest/gtest.h>
#include "utils/include/config_manager.h"

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ConfigManager::instance().loadDefaults();
    }
};

TEST_F(ConfigManagerTest, DefaultValues) {
    auto& config = ConfigManager::instance();
    
    EXPECT_EQ(config.getSerialBaudRate(), 115200);
    EXPECT_FLOAT_EQ(config.getMaxHeight(), 100.0f);
    EXPECT_FLOAT_EQ(config.getMaxAngle(), 45.0f);
}

TEST_F(ConfigManagerTest, SetAndGetValues) {
    auto& config = ConfigManager::instance();
    
    config.setSerialPort("COM5");
    EXPECT_EQ(config.getSerialPort(), "COM5");
    
    config.setMaxHeight(150.0f);
    EXPECT_FLOAT_EQ(config.getMaxHeight(), 150.0f);
}