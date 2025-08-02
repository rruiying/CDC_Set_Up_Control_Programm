// tests/utils_tests/test_logger.cpp
#include <gtest/gtest.h>
#include "utils/include/logger.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <regex>

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清理单例状态
        Logger::getInstance().reset();
        
        // 创建测试日志目录
        testLogDir = "test_logs";
        if (!std::filesystem::exists(testLogDir)) {
            std::filesystem::create_directory(testLogDir);
        }
        
        testLogFile = testLogDir + "/test.log";
    }
    
    void TearDown() override {
        // 清理测试文件
        if (std::filesystem::exists(testLogFile)) {
            // 确保文件被关闭
            Logger::getInstance().close();
            std::filesystem::remove(testLogFile);
        }
        
        // 清理测试目录
        if (std::filesystem::exists(testLogDir)) {
            std::filesystem::remove(testLogDir);
        }
    }
    
    std::string readLogFile() {
        std::ifstream file(testLogFile);
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }
    
    int countLinesInFile() {
        std::ifstream file(testLogFile);
        return std::count(std::istreambuf_iterator<char>(file),
                         std::istreambuf_iterator<char>(), '\n');
    }
    
    std::string testLogDir;
    std::string testLogFile;
};

// 测试单例模式
TEST_F(LoggerTest, SingletonPattern) {
    Logger& logger1 = Logger::getInstance();
    Logger& logger2 = Logger::getInstance();
    
    // 应该是同一个实例
    EXPECT_EQ(&logger1, &logger2);
}

// 测试基本日志记录
TEST_F(LoggerTest, BasicLogging) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile(testLogFile);
    logger.setConsoleOutput(false);  // 测试时关闭控制台输出
    
    logger.info("Test info message");
    logger.warning("Test warning message");
    logger.error("Test error message");
    
    // 等待一下确保写入完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readLogFile();
    
    // 验证日志内容
    EXPECT_TRUE(content.find("[INFO] Test info message") != std::string::npos);
    EXPECT_TRUE(content.find("[WARNING] Test warning message") != std::string::npos);
    EXPECT_TRUE(content.find("[ERROR] Test error message") != std::string::npos);
}

// 测试日志级别过滤
TEST_F(LoggerTest, LogLevelFiltering) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile(testLogFile);
    logger.setConsoleOutput(false);
    
    // 设置为只记录WARNING及以上
    logger.setLogLevel(LogLevel::WARNING);
    
    logger.info("This should not appear");
    logger.warning("This should appear");
    logger.error("This should also appear");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readLogFile();
    
    EXPECT_TRUE(content.find("This should not appear") == std::string::npos);
    EXPECT_TRUE(content.find("This should appear") != std::string::npos);
    EXPECT_TRUE(content.find("This should also appear") != std::string::npos);
}

// 测试时间戳格式
TEST_F(LoggerTest, TimestampFormat) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile(testLogFile);
    logger.setConsoleOutput(false);
    
    logger.info("Test message");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readLogFile();
    
    // 验证时间戳格式 [YYYY-MM-DD HH:MM:SS.mmm]
    std::regex timestampRegex(R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}\])");
    EXPECT_TRUE(std::regex_search(content, timestampRegex));
}

// 测试线程安全
TEST_F(LoggerTest, ThreadSafety) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile(testLogFile);
    logger.setConsoleOutput(false);
    
    const int numThreads = 10;
    const int messagesPerThread = 100;
    
    std::vector<std::thread> threads;
    
    // 启动多个线程同时写日志
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&logger, i, messagesPerThread]() {
            for (int j = 0; j < messagesPerThread; ++j) {
                logger.info("Thread " + std::to_string(i) + " message " + std::to_string(j));
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 验证所有消息都被记录
    int lineCount = countLinesInFile();
    EXPECT_EQ(lineCount, numThreads * messagesPerThread);
}

// 测试内存日志缓冲
TEST_F(LoggerTest, MemoryBuffer) {
    Logger& logger = Logger::getInstance();
    logger.setConsoleOutput(false);
    logger.setMemoryBufferSize(5);  // 只保留最近5条
    
    // 记录10条日志
    for (int i = 0; i < 10; ++i) {
        logger.info("Message " + std::to_string(i));
    }
    
    auto recentLogs = logger.getRecentLogs();
    
    // 应该只有最近的5条
    EXPECT_EQ(recentLogs.size(), 5);
    
    // 验证是最新的5条（5-9）
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(recentLogs[i].message.find("Message " + std::to_string(i + 5)) != std::string::npos);
    }
}

// 测试日志条目的级别过滤
TEST_F(LoggerTest, GetLogsByLevel) {
    Logger& logger = Logger::getInstance();
    logger.setConsoleOutput(false);
    logger.setMemoryBufferSize(100);
    
    // 记录不同级别的日志
    logger.info("Info 1");
    logger.warning("Warning 1");
    logger.error("Error 1");
    logger.info("Info 2");
    logger.warning("Warning 2");
    
    // 获取特定级别的日志
    auto warningLogs = logger.getLogsByLevel(LogLevel::WARNING);
    
    EXPECT_EQ(warningLogs.size(), 2);
    EXPECT_TRUE(warningLogs[0].message.find("Warning 1") != std::string::npos);
    EXPECT_TRUE(warningLogs[1].message.find("Warning 2") != std::string::npos);
}

// 测试清空日志
TEST_F(LoggerTest, ClearLogs) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile(testLogFile);
    logger.setConsoleOutput(false);
    logger.setMemoryBufferSize(100);
    
    // 记录一些日志
    logger.info("Message 1");
    logger.info("Message 2");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 清空内存中的日志
    logger.clearMemoryLogs();
    auto logs = logger.getRecentLogs();
    EXPECT_EQ(logs.size(), 0);
    
    // 文件中的日志应该还在
    EXPECT_GT(countLinesInFile(), 0);
}

// 测试日志文件切换
TEST_F(LoggerTest, ChangeLogFile) {
    Logger& logger = Logger::getInstance();
    logger.setConsoleOutput(false);
    
    // 写入第一个文件
    logger.setLogFile(testLogFile);
    logger.info("Message in first file");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 切换到新文件
    std::string secondFile = testLogDir + "/test2.log";
    logger.setLogFile(secondFile);
    logger.info("Message in second file");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 验证两个文件的内容
    std::string content1 = readLogFile();
    EXPECT_TRUE(content1.find("Message in first file") != std::string::npos);
    
    std::ifstream file2(secondFile);
    std::string content2((std::istreambuf_iterator<char>(file2)),
                        std::istreambuf_iterator<char>());
    EXPECT_TRUE(content2.find("Message in second file") != std::string::npos);
    
    // 清理第二个文件
    logger.close();
    std::filesystem::remove(secondFile);
}

// 测试格式化日志
TEST_F(LoggerTest, FormattedLogging) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile(testLogFile);
    logger.setConsoleOutput(false);
    
    // 测试带格式的日志
    logger.info("Device connected: %s, Port: %s, Baud: %d", "Motor Controller", "COM3", 115200);
    logger.error("Sensor reading failed: %s (code: %d)", "Timeout", -1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readLogFile();
    
    EXPECT_TRUE(content.find("Device connected: Motor Controller, Port: COM3, Baud: 115200") != std::string::npos);
    EXPECT_TRUE(content.find("Sensor reading failed: Timeout (code: -1)") != std::string::npos);
}

// 测试系统操作日志
TEST_F(LoggerTest, SystemOperationLogging) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile(testLogFile);
    logger.setConsoleOutput(false);
    
    // 记录系统操作
    logger.logOperation("MOTOR_MOVE", "Height: 25mm, Angle: 5.5°");
    logger.logOperation("DATA_RECORD", "Recorded 15 data points");
    logger.logOperation("EMERGENCY_STOP", "User triggered emergency stop");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readLogFile();
    
    // 验证操作日志格式
    EXPECT_TRUE(content.find("[OPERATION] MOTOR_MOVE") != std::string::npos);
    EXPECT_TRUE(content.find("Height: 25mm, Angle: 5.5°") != std::string::npos);
}

// 测试通信日志
TEST_F(LoggerTest, CommunicationLogging) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile(testLogFile);
    logger.setConsoleOutput(false);
    
    // 记录通信日志
    logger.logCommunication("TX", "COM3", "SET_HEIGHT:25");
    logger.logCommunication("RX", "COM3", "OK:HEIGHT_SET");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string content = readLogFile();
    
    // 验证通信日志格式
    EXPECT_TRUE(content.find("[COMM] TX [COM3]") != std::string::npos);
    EXPECT_TRUE(content.find("SET_HEIGHT:25") != std::string::npos);
    EXPECT_TRUE(content.find("[COMM] RX [COM3]") != std::string::npos);
}