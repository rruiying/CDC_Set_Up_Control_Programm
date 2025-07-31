#include <gtest/gtest.h>
#include "utils/include/logger.h"
#include <QFile>

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 使用测试专用日志文件
        Logger::instance().setLogFile("test_log.txt");
    }
    
    void TearDown() override {
        // 清理测试文件
        QFile::remove("test_log.txt");
    }
};

TEST_F(LoggerTest, LogInfo) {
    Logger::instance().log(Logger::Info, "Test info message");
    
    // 检查日志文件是否包含消息
    QFile file("test_log.txt");
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    
    EXPECT_TRUE(content.contains("INFO"));
    EXPECT_TRUE(content.contains("Test info message"));
}

TEST_F(LoggerTest, LogLevels) {
    Logger& logger = Logger::instance();
    
    logger.log(Logger::Debug, "Debug message");
    logger.log(Logger::Warning, "Warning message");
    logger.log(Logger::Error, "Error message");
    
    // 简单测试：不崩溃就算通过
    SUCCEED();
}