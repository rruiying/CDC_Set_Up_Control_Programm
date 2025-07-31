#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include "data/include/export_manager.h"
#include "models/include/sensor_data.h"

class ExportManagerTest : public ::testing::Test {
protected:
    ExportManager* exporter;
    QTemporaryDir* tempDir;
    
    void SetUp() override {
        exporter = new ExportManager();
        tempDir = new QTemporaryDir();
    }
    
    void TearDown() override {
        delete exporter;
        delete tempDir;
    }
    
    QString getTempFilePath(const QString& fileName) {
        return tempDir->path() + "/" + fileName;
    }
    
    QList<SensorData> createTestDataList(int count) {
        QList<SensorData> list;
        for (int i = 0; i < count; ++i) {
            SensorData data;
            data.temperature = 20.0f + i;
            data.angle = 5.0f + i * 0.5f;
            data.capacitance = 100.0f + i * 2.0f;
            data.timestamp = QDateTime::currentDateTime().addSecs(i);
            data.isValid.temperature = true;
            data.isValid.angle = true;
            data.isValid.capacitance = true;
            list.append(data);
        }
        return list;
    }
};

// 测试CSV导出
TEST_F(ExportManagerTest, ExportToCSV) {
    QString fileName = getTempFilePath("test_export.csv");
    QList<SensorData> dataList = createTestDataList(5);
    
    bool result = exporter->exportToCSV(dataList, fileName);
    EXPECT_TRUE(result);
    
    // 验证文件存在
    EXPECT_TRUE(QFile::exists(fileName));
    
    // 验证文件内容
    QFile file(fileName);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    
    // 检查头部
    EXPECT_TRUE(content.contains("Timestamp"));
    EXPECT_TRUE(content.contains("Temperature"));
    
    // 检查数据
    EXPECT_TRUE(content.contains("20"));
    EXPECT_TRUE(content.contains("21"));
}

// 测试JSON导出
TEST_F(ExportManagerTest, ExportToJSON) {
    QString fileName = getTempFilePath("test_export.json");
    QList<SensorData> dataList = createTestDataList(3);
    
    bool result = exporter->exportToJSON(dataList, fileName);
    EXPECT_TRUE(result);
    
    // 验证文件存在
    EXPECT_TRUE(QFile::exists(fileName));
    
    // 验证JSON格式
    QFile file(fileName);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    
    EXPECT_FALSE(doc.isNull());
    EXPECT_TRUE(doc.object().contains("data"));
    EXPECT_EQ(doc.object()["count"].toInt(), 3);
}

// 测试导出选项
TEST_F(ExportManagerTest, ExportWithOptions) {
    QString fileName = getTempFilePath("test_options.csv");
    QList<SensorData> dataList = createTestDataList(2);
    
    ExportManager::ExportOptions options;
    options.format = ExportManager::ExportFormat::CSV;
    options.includeHeader = true;
    options.includeTimestamp = true;
    options.precision = 2;
    
    bool result = exporter->exportData(dataList, fileName, options);
    EXPECT_TRUE(result);
}

// 测试导出信号
TEST_F(ExportManagerTest, ExportSignals) {
    QSignalSpy startSpy(exporter, &ExportManager::exportStarted);
    QSignalSpy completeSpy(exporter, &ExportManager::exportCompleted);
    QSignalSpy progressSpy(exporter, &ExportManager::exportProgress);
    
    QString fileName = getTempFilePath("test_signals.csv");
    QList<SensorData> dataList = createTestDataList(10);
    
    exporter->exportToCSV(dataList, fileName);
    
    EXPECT_EQ(startSpy.count(), 1);
    EXPECT_EQ(completeSpy.count(), 1);
    EXPECT_GT(progressSpy.count(), 0);
}

// 测试报告生成
TEST_F(ExportManagerTest, ExportReport) {
    QString fileName = getTempFilePath("test_report.txt");
    QList<SensorData> dataList = createTestDataList(10);
    
    bool result = exporter->exportReport(dataList, fileName);
    EXPECT_TRUE(result);
    
    // 验证报告内容
    QFile file(fileName);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    
    EXPECT_TRUE(content.contains("CDC Measurement Report"));
    EXPECT_TRUE(content.contains("Total Records: 10"));
    EXPECT_TRUE(content.contains("Average Temperature"));
}

// 测试空数据导出
TEST_F(ExportManagerTest, ExportEmptyData) {
    QString fileName = getTempFilePath("test_empty.csv");
    QList<SensorData> emptyList;
    
    bool result = exporter->exportToCSV(emptyList, fileName);
    EXPECT_TRUE(result);
    
    // 文件应该只包含头部
    QFile file(fileName);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    EXPECT_TRUE(content.contains("Timestamp"));
}

// 测试无效文件路径
TEST_F(ExportManagerTest, ExportToInvalidPath) {
    QString fileName = "/invalid/path/test.csv";
    QList<SensorData> dataList = createTestDataList(5);
    
    QSignalSpy failSpy(exporter, &ExportManager::exportFailed);
    
    bool result = exporter->exportToCSV(dataList, fileName);
    EXPECT_FALSE(result);
    EXPECT_EQ(failSpy.count(), 1);
}