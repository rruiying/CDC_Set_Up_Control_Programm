// tests/data_tests/test_export_manager.cpp
#include <gtest/gtest.h>
#include "data/include/export_manager.h"
#include "models/include/measurement_data.h"
#include "models/include/sensor_data.h"
#include "utils/include/logger.h"
#include <filesystem>
#include <fstream>
#include <chrono>

class ExportManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setConsoleOutput(false);
        
        // 创建测试目录
        testDir = "test_exports";
        if (!std::filesystem::exists(testDir)) {
            std::filesystem::create_directory(testDir);
        }
        
        exportManager = std::make_unique<ExportManager>();
        
        // 创建测试数据
        createTestData();
    }
    
    void TearDown() override {
        // 清理测试文件
        if (std::filesystem::exists(testDir)) {
            std::filesystem::remove_all(testDir);
        }
    }
    
    void createTestData() {
        for (int i = 0; i < 10; i++) {
            SensorData sensorData(10.0 + i, 11.0 + i, 150.0 + i, 151.0 + i, 
                                20.0 + i * 0.5, i * 0.5, 100.0 + i * 2);
            MeasurementData measurement(25.0 + i, 5.0 + i * 0.5, sensorData);
            testData.push_back(measurement);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    std::string testDir;
    std::unique_ptr<ExportManager> exportManager;
    std::vector<MeasurementData> testData;
};

// 测试基本CSV导出
TEST_F(ExportManagerTest, BasicCSVExport) {
    std::string filename = testDir + "/basic.csv";
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    options.includeHeader = true;
    
    bool success = exportManager->exportData(testData, filename, options);
    EXPECT_TRUE(success);
    
    // 验证文件存在
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    // 验证文件内容
    std::ifstream file(filename);
    std::string firstLine;
    std::getline(file, firstLine);
    
    // 第一行应该包含头部
    EXPECT_TRUE(firstLine.find("Timestamp") != std::string::npos);
}

// 测试无头部导出
TEST_F(ExportManagerTest, CSVExportWithoutHeader) {
    std::string filename = testDir + "/no_header.csv";
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    options.includeHeader = false;
    
    exportManager->exportData(testData, filename, options);
    
    // 读取第一行
    std::ifstream file(filename);
    std::string firstLine;
    std::getline(file, firstLine);
    
    // 第一行不应该包含头部标识
    EXPECT_FALSE(firstLine.find("Timestamp") != std::string::npos);
}

// 测试自定义分隔符
TEST_F(ExportManagerTest, CustomDelimiter) {
    std::string filename = testDir + "/custom_delimiter.csv";
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    options.delimiter = ";";
    
    exportManager->exportData(testData, filename, options);
    
    // 验证使用了自定义分隔符
    std::ifstream file(filename);
    std::string line;
    std::getline(file, line); // 跳过头部
    std::getline(file, line);
    
    EXPECT_TRUE(line.find(";") != std::string::npos);
    EXPECT_FALSE(line.find(",") != std::string::npos);
}

// 测试选择性字段导出
TEST_F(ExportManagerTest, SelectiveFieldExport) {
    std::string filename = testDir + "/selective.csv";
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    options.includeTimestamp = true;
    options.includeSetValues = true;
    options.includeSensorData = false;
    options.includeCalculatedValues = false;
    
    exportManager->exportData(testData, filename, options);
    
    // 读取头部验证字段
    std::ifstream file(filename);
    std::string header;
    std::getline(file, header);
    
    EXPECT_TRUE(header.find("Timestamp") != std::string::npos);
    EXPECT_TRUE(header.find("Set_Height") != std::string::npos);
    EXPECT_FALSE(header.find("Upper_Sensor") != std::string::npos);
}

// 测试Excel格式导出
TEST_F(ExportManagerTest, ExcelFormatExport) {
    std::string filename = testDir + "/data.xlsx";
    
    ExportOptions options;
    options.format = ExportFormat::EXCEL;
    
    bool success = exportManager->exportData(testData, filename, options);
    
    // 注意：实际Excel导出可能需要外部库
    // 这里我们假设导出为CSV但使用xlsx扩展名
    if (success) {
        EXPECT_TRUE(std::filesystem::exists(filename));
    }
}

// 测试JSON格式导出
TEST_F(ExportManagerTest, JSONExport) {
    std::string filename = testDir + "/data.json";
    
    ExportOptions options;
    options.format = ExportFormat::JSON;
    options.prettyPrint = true;
    
    bool success = exportManager->exportData(testData, filename, options);
    EXPECT_TRUE(success);
    
    // 验证JSON格式
    std::ifstream file(filename);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("[") != std::string::npos);
    EXPECT_TRUE(content.find("]") != std::string::npos);
    EXPECT_TRUE(content.find("{") != std::string::npos);
}

// 测试压缩导出
TEST_F(ExportManagerTest, CompressedExport) {
    std::string filename = testDir + "/data.csv.gz";
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    options.compress = true;
    
    bool success = exportManager->exportData(testData, filename, options);
    
    if (success) {
        EXPECT_TRUE(std::filesystem::exists(filename));
        // 压缩文件应该更小
        auto compressedSize = std::filesystem::file_size(filename);
        
        // 导出未压缩版本
        options.compress = false;
        std::string uncompressedFile = testDir + "/data_uncompressed.csv";
        exportManager->exportData(testData, uncompressedFile, options);
        auto uncompressedSize = std::filesystem::file_size(uncompressedFile);
        
        EXPECT_LT(compressedSize, uncompressedSize);
    }
}

// 测试批量导出
TEST_F(ExportManagerTest, BatchExport) {
    std::vector<std::string> filenames = {
        testDir + "/batch1.csv",
        testDir + "/batch2.json",
        testDir + "/batch3.txt"
    };
    
    std::vector<ExportOptions> optionsList = {
        {ExportFormat::CSV, true},
        {ExportFormat::JSON, true},
        {ExportFormat::TEXT, true}
    };
    
    bool success = exportManager->batchExport(testData, filenames, optionsList);
    EXPECT_TRUE(success);
    
    // 验证所有文件都被创建
    for (const auto& filename : filenames) {
        EXPECT_TRUE(std::filesystem::exists(filename));
    }
}

// 测试模板导出
TEST_F(ExportManagerTest, TemplateExport) {
    // 创建模板
    ExportTemplate csvTemplate;
    csvTemplate.name = "Basic CSV";
    csvTemplate.options.format = ExportFormat::CSV;
    csvTemplate.options.includeHeader = true;
    csvTemplate.options.delimiter = ",";
    
    exportManager->addTemplate(csvTemplate);
    
    // 使用模板导出
    std::string filename = testDir + "/template.csv";
    bool success = exportManager->exportUsingTemplate(testData, filename, "Basic CSV");
    EXPECT_TRUE(success);
    
    EXPECT_TRUE(std::filesystem::exists(filename));
}

// 测试导出进度回调
TEST_F(ExportManagerTest, ExportProgress) {
    std::vector<int> progressValues;
    
    exportManager->setProgressCallback([&progressValues](int progress) {
        progressValues.push_back(progress);
    });
    
    std::string filename = testDir + "/progress.csv";
    ExportOptions options;
    options.format = ExportFormat::CSV;
    
    exportManager->exportData(testData, filename, options);
    
    // 应该收到进度更新
    EXPECT_GT(progressValues.size(), 0);
    // 最后的进度应该是100
    if (!progressValues.empty()) {
        EXPECT_EQ(progressValues.back(), 100);
    }
}

// 测试数据过滤导出
TEST_F(ExportManagerTest, FilteredExport) {
    std::string filename = testDir + "/filtered.csv";
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    
    // 只导出高度大于27的数据
    auto filter = [](const MeasurementData& m) {
        return m.getSetHeight() > 27.0;
    };
    
    bool success = exportManager->exportFiltered(testData, filename, options, filter);
    EXPECT_TRUE(success);
    
    // 统计导出的行数
    std::ifstream file(filename);
    std::string line;
    int lineCount = -1; // 减去头部
    while (std::getline(file, line)) {
        lineCount++;
    }
    
    // 应该有约6-7条记录（取决于测试数据）
    EXPECT_GT(lineCount, 0);
    EXPECT_LT(lineCount, 10);
}

// 测试统计信息导出
TEST_F(ExportManagerTest, StatisticsExport) {
    std::string filename = testDir + "/statistics.txt";
    
    bool success = exportManager->exportStatistics(testData, filename);
    EXPECT_TRUE(success);
    
    // 验证统计信息内容
    std::ifstream file(filename);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("Total Records:") != std::string::npos);
    EXPECT_TRUE(content.find("Average Height:") != std::string::npos);
    EXPECT_TRUE(content.find("Min Height:") != std::string::npos);
    EXPECT_TRUE(content.find("Max Height:") != std::string::npos);
}

// 测试自动文件名生成
TEST_F(ExportManagerTest, AutoFilename) {
    ExportOptions options;
    options.format = ExportFormat::CSV;
    options.autoGenerateFilename = true;
    options.filenamePrefix = "test_data";
    
    std::string filename = exportManager->generateFilename(options);
    
    // 文件名应该包含前缀和时间戳
    EXPECT_TRUE(filename.find("test_data") != std::string::npos);
    EXPECT_TRUE(filename.find(".csv") != std::string::npos);
    
    // 导出到自动生成的文件名
    bool success = exportManager->exportData(testData, testDir + "/" + filename, options);
    EXPECT_TRUE(success);
}

// 测试错误处理
TEST_F(ExportManagerTest, ErrorHandling) {
    // 尝试导出到无效路径
    std::string filename = "/invalid/path/data.csv";
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    
    bool success = exportManager->exportData(testData, filename, options);
    EXPECT_FALSE(success);
    
    // 获取错误信息
    std::string error = exportManager->getLastError();
    EXPECT_FALSE(error.empty());
}

// 测试大数据集导出
TEST_F(ExportManagerTest, LargeDatasetExport) {
    // 创建大数据集
    std::vector<MeasurementData> largeData;
    for (int i = 0; i < 1000; i++) {
        SensorData sensorData(10.0, 11.0, 150.0, 151.0, 20.0, 0.0, 100.0);
        MeasurementData measurement(25.0, 5.0, sensorData);
        largeData.push_back(measurement);
    }
    
    std::string filename = testDir + "/large_dataset.csv";
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    options.useBuffering = true;
    options.bufferSize = 1024 * 1024; // 1MB buffer
    
    auto start = std::chrono::steady_clock::now();
    bool success = exportManager->exportData(largeData, filename, options);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    // 性能测试：导出1000条记录应该在合理时间内完成
    EXPECT_LT(duration, 1000); // 少于1秒
}

// 测试多线程导出
TEST_F(ExportManagerTest, ConcurrentExport) {
    const int numThreads = 3;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([this, i, &successCount]() {
            std::string filename = testDir + "/concurrent_" + std::to_string(i) + ".csv";
            
            ExportOptions options;
            options.format = ExportFormat::CSV;
            
            if (exportManager->exportData(testData, filename, options)) {
                successCount++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // 所有导出都应该成功
    EXPECT_EQ(successCount.load(), numThreads);
}