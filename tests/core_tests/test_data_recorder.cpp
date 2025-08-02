// tests/core_tests/test_data_recorder.cpp
#include <gtest/gtest.h>
#include "core/include/data_recorder.h"
#include "models/include/measurement_data.h"
#include "models/include/sensor_data.h"
#include "models/include/system_config.h"
#include "utils/include/logger.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

class DataRecorderTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setConsoleOutput(false);
        SystemConfig::getInstance().reset();
        
        // 创建测试目录
        testDir = "test_data";
        if (!std::filesystem::exists(testDir)) {
            std::filesystem::create_directory(testDir);
        }
        
        dataRecorder = std::make_unique<DataRecorder>();
    }
    
    void TearDown() override {
        // 清理测试文件
        if (std::filesystem::exists(testDir)) {
            std::filesystem::remove_all(testDir);
        }
    }
    
    MeasurementData createTestMeasurement(double height, double angle) {
        SensorData sensorData(12.5, 13.0, 156.2, 156.8, 23.5, 2.5, 157.3);
        return MeasurementData(height, angle, sensorData);
    }
    
    std::string testDir;
    std::unique_ptr<DataRecorder> dataRecorder;
};

// 测试基本记录功能
TEST_F(DataRecorderTest, BasicRecording) {
    // 添加测量数据
    auto measurement = createTestMeasurement(25.0, 5.5);
    dataRecorder->recordMeasurement(measurement);
    
    // 验证记录计数
    EXPECT_EQ(dataRecorder->getRecordCount(), 1);
    EXPECT_TRUE(dataRecorder->hasData());
    
    // 获取最新记录
    auto latest = dataRecorder->getLatestMeasurement();
    EXPECT_DOUBLE_EQ(latest.getSetHeight(), 25.0);
    EXPECT_DOUBLE_EQ(latest.getSetAngle(), 5.5);
}

// 测试多条记录
TEST_F(DataRecorderTest, MultipleRecords) {
    // 添加多条记录
    for (int i = 0; i < 10; i++) {
        auto measurement = createTestMeasurement(20.0 + i, i * 0.5);
        dataRecorder->recordMeasurement(measurement);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    EXPECT_EQ(dataRecorder->getRecordCount(), 10);
    
    // 获取所有记录
    auto allRecords = dataRecorder->getAllMeasurements();
    EXPECT_EQ(allRecords.size(), 10);
    
    // 验证顺序（最新的在后面）
    EXPECT_DOUBLE_EQ(allRecords.front().getSetHeight(), 20.0);
    EXPECT_DOUBLE_EQ(allRecords.back().getSetHeight(), 29.0);
}

// 测试最大记录限制
TEST_F(DataRecorderTest, MaxRecordsLimit) {
    // 设置最大记录数
    dataRecorder->setMaxRecords(5);
    
    // 添加超过限制的记录
    for (int i = 0; i < 10; i++) {
        auto measurement = createTestMeasurement(20.0 + i, i * 0.5);
        dataRecorder->recordMeasurement(measurement);
    }
    
    // 应该只保留最新的5条
    EXPECT_EQ(dataRecorder->getRecordCount(), 5);
    
    auto records = dataRecorder->getAllMeasurements();
    EXPECT_DOUBLE_EQ(records.front().getSetHeight(), 25.0); // 第6条记录
    EXPECT_DOUBLE_EQ(records.back().getSetHeight(), 29.0);  // 第10条记录
}

// 测试清空记录
TEST_F(DataRecorderTest, ClearRecords) {
    // 添加一些记录
    for (int i = 0; i < 5; i++) {
        dataRecorder->recordMeasurement(createTestMeasurement(20.0 + i, i));
    }
    
    EXPECT_EQ(dataRecorder->getRecordCount(), 5);
    
    // 清空记录
    dataRecorder->clear();
    
    EXPECT_EQ(dataRecorder->getRecordCount(), 0);
    EXPECT_FALSE(dataRecorder->hasData());
}

// 测试CSV导出
TEST_F(DataRecorderTest, ExportToCSV) {
    // 添加测试数据
    for (int i = 0; i < 3; i++) {
        auto measurement = createTestMeasurement(25.0 + i, 5.0 + i);
        dataRecorder->recordMeasurement(measurement);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 导出到CSV
    std::string filename = testDir + "/test_export.csv";
    bool success = dataRecorder->exportToCSV(filename);
    EXPECT_TRUE(success);
    
    // 验证文件存在
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    // 读取并验证内容
    std::ifstream file(filename);
    std::string line;
    
    // 第一行应该是头部
    std::getline(file, line);
    EXPECT_TRUE(line.find("Timestamp") != std::string::npos);
    EXPECT_TRUE(line.find("Set_Height") != std::string::npos);
    
    // 应该有3行数据
    int dataLines = 0;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            dataLines++;
        }
    }
    EXPECT_EQ(dataLines, 3);
}

// 测试自动保存
TEST_F(DataRecorderTest, AutoSave) {
    // 设置自动保存
    std::string autoSaveFile = testDir + "/autosave.csv";
    dataRecorder->setAutoSave(true, autoSaveFile);
    dataRecorder->setAutoSaveInterval(100); // 100ms
    
    // 添加数据
    dataRecorder->recordMeasurement(createTestMeasurement(25.0, 5.0));
    
    // 等待自动保存
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // 验证文件存在
    EXPECT_TRUE(std::filesystem::exists(autoSaveFile));
}

// 测试时间范围查询
TEST_F(DataRecorderTest, TimeRangeQuery) {
    auto now = std::chrono::system_clock::now();
    
    // 添加不同时间的记录
    for (int i = 0; i < 5; i++) {
        auto measurement = createTestMeasurement(20.0 + i, i);
        dataRecorder->recordMeasurement(measurement);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 查询最近200ms的数据
    auto recentData = dataRecorder->getMeasurementsInTimeRange(
        now + std::chrono::milliseconds(200),
        std::chrono::system_clock::now()
    );
    
    // 应该有2-3条记录
    EXPECT_GE(recentData.size(), 2);
    EXPECT_LE(recentData.size(), 3);
}

// 测试数据过滤
TEST_F(DataRecorderTest, DataFiltering) {
    // 添加不同高度的数据
    dataRecorder->recordMeasurement(createTestMeasurement(20.0, 0.0));
    dataRecorder->recordMeasurement(createTestMeasurement(25.0, 5.0));
    dataRecorder->recordMeasurement(createTestMeasurement(30.0, 10.0));
    dataRecorder->recordMeasurement(createTestMeasurement(35.0, 15.0));
    
    // 过滤高度在25-35之间的数据
    auto filtered = dataRecorder->filterMeasurements([](const MeasurementData& m) {
        return m.getSetHeight() >= 25.0 && m.getSetHeight() <= 35.0;
    });
    
    EXPECT_EQ(filtered.size(), 3);
    EXPECT_DOUBLE_EQ(filtered[0].getSetHeight(), 25.0);
    EXPECT_DOUBLE_EQ(filtered[2].getSetHeight(), 35.0);
}

// 测试统计信息
TEST_F(DataRecorderTest, Statistics) {
    // 添加测试数据
    dataRecorder->recordMeasurement(createTestMeasurement(20.0, 0.0));
    dataRecorder->recordMeasurement(createTestMeasurement(30.0, 10.0));
    dataRecorder->recordMeasurement(createTestMeasurement(40.0, 20.0));
    
    auto stats = dataRecorder->getStatistics();
    
    // 验证统计信息
    EXPECT_EQ(stats.totalRecords, 3);
    EXPECT_DOUBLE_EQ(stats.averageHeight, 30.0);
    EXPECT_DOUBLE_EQ(stats.averageAngle, 10.0);
    EXPECT_DOUBLE_EQ(stats.minHeight, 20.0);
    EXPECT_DOUBLE_EQ(stats.maxHeight, 40.0);
    EXPECT_DOUBLE_EQ(stats.minAngle, 0.0);
    EXPECT_DOUBLE_EQ(stats.maxAngle, 20.0);
}

// 测试数据备份
TEST_F(DataRecorderTest, DataBackup) {
    // 添加数据
    for (int i = 0; i < 5; i++) {
        dataRecorder->recordMeasurement(createTestMeasurement(20.0 + i, i));
    }
    
    // 创建备份
    std::string backupFile = testDir + "/backup.csv";
    bool success = dataRecorder->createBackup(backupFile);
    EXPECT_TRUE(success);
    
    // 清空当前数据
    dataRecorder->clear();
    EXPECT_EQ(dataRecorder->getRecordCount(), 0);
    
    // 从备份恢复
    success = dataRecorder->restoreFromBackup(backupFile);
    EXPECT_TRUE(success);
    EXPECT_EQ(dataRecorder->getRecordCount(), 5);
}

// 测试数据变化通知
TEST_F(DataRecorderTest, DataChangeNotification) {
    bool notified = false;
    int recordCount = 0;
    
    // 设置回调
    dataRecorder->setDataChangeCallback([&notified, &recordCount](int count) {
        notified = true;
        recordCount = count;
    });
    
    // 添加数据
    dataRecorder->recordMeasurement(createTestMeasurement(25.0, 5.0));
    
    // 验证回调被触发
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(notified);
    EXPECT_EQ(recordCount, 1);
}

// 测试内存使用限制
TEST_F(DataRecorderTest, MemoryLimit) {
    // 设置内存限制（假设每条记录约1KB）
    dataRecorder->setMemoryLimit(10 * 1024); // 10KB
    
    // 尝试添加大量数据
    for (int i = 0; i < 100; i++) {
        dataRecorder->recordMeasurement(createTestMeasurement(20.0 + i, i));
    }
    
    // 记录数应该被限制
    EXPECT_LT(dataRecorder->getRecordCount(), 100);
    EXPECT_LE(dataRecorder->getEstimatedMemoryUsage(), 10 * 1024);
}

// 测试并发记录
TEST_F(DataRecorderTest, ConcurrentRecording) {
    const int numThreads = 5;
    const int recordsPerThread = 20;
    std::vector<std::thread> threads;
    
    // 启动多个线程同时记录
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < recordsPerThread; j++) {
                auto measurement = createTestMeasurement(i * 10.0 + j, i + j * 0.1);
                dataRecorder->recordMeasurement(measurement);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 验证所有记录都被保存
    EXPECT_EQ(dataRecorder->getRecordCount(), numThreads * recordsPerThread);
}

// 测试导入CSV
TEST_F(DataRecorderTest, ImportFromCSV) {
    // 创建测试CSV文件
    std::string csvFile = testDir + "/import_test.csv";
    std::ofstream file(csvFile);
    file << MeasurementData::getCSVHeader() << "\n";
    
    // 写入测试数据
    for (int i = 0; i < 3; i++) {
        auto measurement = createTestMeasurement(30.0 + i, 10.0 + i);
        file << measurement.toCSV() << "\n";
    }
    file.close();
    
    // 导入数据
    bool success = dataRecorder->importFromCSV(csvFile);
    EXPECT_TRUE(success);
    
    // 验证导入的数据
    EXPECT_EQ(dataRecorder->getRecordCount(), 3);
    auto records = dataRecorder->getAllMeasurements();
    EXPECT_DOUBLE_EQ(records[0].getSetHeight(), 30.0);
    EXPECT_DOUBLE_EQ(records[2].getSetHeight(), 32.0);
}

// 测试数据压缩（去除相似数据）
TEST_F(DataRecorderTest, DataCompression) {
    // 添加相似的数据
    for (int i = 0; i < 10; i++) {
        // 高度只有微小变化
        auto measurement = createTestMeasurement(25.0 + i * 0.01, 5.0);
        dataRecorder->recordMeasurement(measurement);
    }
    
    // 启用数据压缩
    dataRecorder->setCompressionEnabled(true);
    dataRecorder->setCompressionThreshold(0.1); // 0.1mm差异内视为相同
    
    // 压缩数据
    dataRecorder->compressData();
    
    // 相似数据应该被合并
    EXPECT_LT(dataRecorder->getRecordCount(), 10);
}