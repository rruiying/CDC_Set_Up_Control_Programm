#include <gtest/gtest.h>
#include <QSignalSpy>
#include "data/include/data_processor.h"
#include "models/include/sensor_data.h"

class DataProcessorTest : public ::testing::Test {
protected:
    DataProcessor* processor;
    
    void SetUp() override {
        processor = new DataProcessor();
    }
    
    void TearDown() override {
        delete processor;
    }
    
    // 创建测试数据
    SensorData createTestData(float temp, float height, float angle) {
        SensorData data;
        data.temperature = temp;
        data.distanceUpper1 = height;
        data.distanceUpper2 = height + 0.5f;
        data.angle = angle;
        data.isValid.temperature = true;
        data.isValid.distanceUpper1 = true;
        data.isValid.distanceUpper2 = true;
        data.isValid.angle = true;
        return data;
    }
};

// 测试数据处理
TEST_F(DataProcessorTest, ProcessSingleData) {
    QSignalSpy spy(processor, &DataProcessor::dataProcessed);
    
    SensorData input = createTestData(25.0f, 50.0f, 10.0f);
    SensorData output = processor->processData(input);
    
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FLOAT_EQ(output.temperature, 25.0f);
}

// 测试移动平均滤波
TEST_F(DataProcessorTest, MovingAverageFilter) {
    processor->setFilterType(DataProcessor::FilterType::MovingAverage);
    processor->setWindowSize(3);
    
    // 发送多个数据点
    processor->processData(createTestData(20.0f, 50.0f, 10.0f));
    processor->processData(createTestData(22.0f, 50.0f, 10.0f));
    SensorData result = processor->processData(createTestData(24.0f, 50.0f, 10.0f));
    
    // 平均值应该是 (20+22+24)/3 = 22
    EXPECT_NEAR(result.temperature, 22.0f, 0.1f);
}

// 测试指数平滑
TEST_F(DataProcessorTest, ExponentialSmoothing) {
    processor->setFilterType(DataProcessor::FilterType::ExponentialSmoothing);
    processor->setSmoothingFactor(0.5f);
    
    processor->processData(createTestData(20.0f, 50.0f, 10.0f));
    SensorData result = processor->processData(createTestData(30.0f, 50.0f, 10.0f));
    
    // 0.5 * 30 + 0.5 * 20 = 25
    EXPECT_FLOAT_EQ(result.temperature, 25.0f);
}

// 测试异常值检测
TEST_F(DataProcessorTest, OutlierDetection) {
    QSignalSpy spy(processor, &DataProcessor::outlierDetected);
    
    DataProcessor::ProcessingConfig config;
    config.enableOutlierRemoval = true;
    config.outlierThreshold = 2.0f;
    processor->setConfig(config);
    
    // 添加正常数据
    for (int i = 0; i < 10; ++i) {
        processor->processData(createTestData(25.0f + i * 0.1f, 50.0f, 10.0f));
    }
    
    // 添加异常值
    processor->processData(createTestData(100.0f, 50.0f, 10.0f));
    
    EXPECT_GT(spy.count(), 0);
}

// 测试批处理
TEST_F(DataProcessorTest, BatchProcessing) {
    QList<SensorData> inputList;
    for (int i = 0; i < 5; ++i) {
        inputList.append(createTestData(20.0f + i, 50.0f, 10.0f));
    }
    
    QList<SensorData> outputList = processor->processBatch(inputList);
    
    EXPECT_EQ(outputList.size(), inputList.size());
}

// 测试统计计算
TEST_F(DataProcessorTest, StatisticalCalculations) {
    QList<float> values = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f};
    
    float avg = processor->calculateAverage(values);
    EXPECT_FLOAT_EQ(avg, 30.0f);
    
    float stdDev = processor->calculateStdDev(values);
    EXPECT_NEAR(stdDev, 15.81f, 0.1f);
    
    auto [min, max] = processor->calculateMinMax(values);
    EXPECT_FLOAT_EQ(min, 10.0f);
    EXPECT_FLOAT_EQ(max, 50.0f);
}

// 测试中值滤波
TEST_F(DataProcessorTest, MedianFilter) {
    processor->setFilterType(DataProcessor::FilterType::MedianFilter);
    processor->setWindowSize(5);
    
    // 添加数据：10, 20, 100, 30, 40
    processor->processData(createTestData(10.0f, 50.0f, 10.0f));
    processor->processData(createTestData(20.0f, 50.0f, 10.0f));
    processor->processData(createTestData(100.0f, 50.0f, 10.0f)); // 异常值
    processor->processData(createTestData(30.0f, 50.0f, 10.0f));
    SensorData result = processor->processData(createTestData(40.0f, 50.0f, 10.0f));
    
    // 中值应该是30（排序后：10, 20, 30, 40, 100）
    EXPECT_NEAR(result.temperature, 30.0f, 1.0f);
}