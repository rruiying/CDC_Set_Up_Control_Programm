#include <gtest/gtest.h>
#include <QTemporaryFile>
#include <QTextStream>
#include "core/include/data_recorder.h"
#include "models/include/sensor_data.h"

class DataRecorderTest : public ::testing::Test {
protected:
    DataRecorder* recorder;
    QTemporaryFile* tempFile;
    
    void SetUp() override {
        recorder = new DataRecorder();
        tempFile = new QTemporaryFile();
        tempFile->open();
    }
    
    void TearDown() override {
        delete recorder;
        delete tempFile;
    }
};

// 测试开始记录
TEST_F(DataRecorderTest, StartRecording) {
    bool result = recorder->startRecording(tempFile->fileName());
    EXPECT_TRUE(result);
    EXPECT_TRUE(recorder->isRecording());
}

// 测试停止记录
TEST_F(DataRecorderTest, StopRecording) {
    recorder->startRecording(tempFile->fileName());
    recorder->stopRecording();
    
    EXPECT_FALSE(recorder->isRecording());
}

// 测试记录数据
TEST_F(DataRecorderTest, RecordData) {
    recorder->startRecording(tempFile->fileName());
    
    SensorData data;
    data.temperature = 25.5f;
    data.angle = 15.0f;
    data.timestamp = QDateTime::currentDateTime();
    
    recorder->recordData(data);
    recorder->stopRecording();
    
    // 验证文件内容
    QFile file(tempFile->fileName());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    
    EXPECT_TRUE(content.contains("25.5"));
    EXPECT_TRUE(content.contains("15.0"));
}

// 测试自动文件名生成
TEST_F(DataRecorderTest, AutoFileName) {
    QString fileName = recorder->generateFileName();
    
    EXPECT_TRUE(fileName.contains("CDC_Data_"));
    EXPECT_TRUE(fileName.endsWith(".csv"));
}

// 测试记录限制
TEST_F(DataRecorderTest, RecordingLimits) {
    recorder->setMaxFileSize(1024); // 1KB
    recorder->setMaxDuration(60);   // 60秒
    
    EXPECT_EQ(recorder->getMaxFileSize(), 1024);
    EXPECT_EQ(recorder->getMaxDuration(), 60);
}

// 测试数据缓冲
TEST_F(DataRecorderTest, DataBuffering) {
    recorder->setBufferSize(10);
    recorder->startRecording(tempFile->fileName());
    
    // 添加5条数据（不应该立即写入）
    for (int i = 0; i < 5; ++i) {
        SensorData data;
        data.temperature = 20.0f + i;
        recorder->recordData(data);
    }
    
    EXPECT_EQ(recorder->getBufferedCount(), 5);
    
    // 刷新缓冲区
    recorder->flushBuffer();
    EXPECT_EQ(recorder->getBufferedCount(), 0);
}