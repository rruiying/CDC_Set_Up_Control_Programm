// tests/data_tests/test_data_processor.cpp
#include <gtest/gtest.h>
#include "data/include/data_processor.h"
#include "models/include/measurement_data.h"
#include "models/include/sensor_data.h"
#include "utils/include/logger.h"
#include <cmath>
#include <random>

class DataProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setConsoleOutput(false);
        
        dataProcessor = std::make_unique<DataProcessor>();
        
        // 创建测试数据
        createTestData();
    }
    
    void createTestData() {
        // 创建线性数据
        for (int i = 0; i < 10; i++) {
            double height = 20.0 + i * 5.0;
            double angle = i * 2.0;
            double capacitance = 100.0 + i * 10.0; // 线性关系
            
            SensorData sensorData(10.0 + i, 11.0 + i, 150.0, 151.0, 
                                25.0, angle, capacitance);
            MeasurementData measurement(height, angle, sensorData);
            linearData.push_back(measurement);
        }
        
        // 创建带噪声的数据
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<> noise(0.0, 2.0);
        
        for (int i = 0; i < 20; i++) {
            double height = 25.0 + i * 2.0 + noise(gen);
            double angle = 5.0 + i * 0.5 + noise(gen) * 0.5;
            double capacitance = 150.0 - i * 5.0 + noise(gen) * 3.0;
            
            SensorData sensorData(12.0, 13.0, 155.0, 156.0, 
                                25.0, angle, capacitance);
            MeasurementData measurement(height, angle, sensorData);
            noisyData.push_back(measurement);
        }
    }
    
    std::unique_ptr<DataProcessor> dataProcessor;
    std::vector<MeasurementData> linearData;
    std::vector<MeasurementData> noisyData;
};

// 测试基本统计分析
TEST_F(DataProcessorTest, BasicStatistics) {
    auto trend = dataProcessor->analyzeTrend(linearData, DataField::CAPACITANCE);
    
    // 线性数据应该显示上升趋势
    EXPECT_EQ(trend.direction, TrendDirection::INCREASING);
    EXPECT_GT(trend.strength, 0.8); // 强趋势
    EXPECT_NEAR(trend.changeRate, 2.0, 0.1); // 每单位高度增加2pF
}

// 测试图表数据准备
TEST_F(DataProcessorTest, ChartDataPreparation) {
    // 准备2D散点图数据
    auto scatterData = dataProcessor->prepareScatterPlotData(
        linearData, DataField::HEIGHT, DataField::CAPACITANCE);
    
    EXPECT_EQ(scatterData.xValues.size(), linearData.size());
    EXPECT_EQ(scatterData.yValues.size(), linearData.size());
    EXPECT_EQ(scatterData.xLabel, "Height (mm)");
    EXPECT_EQ(scatterData.yLabel, "Capacitance (pF)");
    
    // 准备3D图表数据
    auto surface3D = dataProcessor->prepare3DSurfaceData(
        linearData, DataField::HEIGHT, DataField::ANGLE, DataField::CAPACITANCE);
    
    EXPECT_GT(surface3D.xGrid.size(), 0);
    EXPECT_GT(surface3D.yGrid.size(), 0);
    EXPECT_GT(surface3D.zValues.size(), 0);
}

// 测试误差分析
TEST_F(DataProcessorTest, ErrorAnalysis) {
    // 创建理论值和测量值数据
    std::vector<double> theoretical;
    std::vector<double> measured;
    
    for (const auto& data : linearData) {
        theoretical.push_back(data.getTheoreticalCapacitance());
        measured.push_back(data.getSensorData().getMeasuredCapacitance());
    }
    
    auto errorStats = dataProcessor->analyzeError(theoretical, measured);
    
    // 验证误差统计
    EXPECT_GE(errorStats.meanAbsoluteError, 0.0);
    EXPECT_GE(errorStats.rootMeanSquareError, 0.0);
    EXPECT_LE(std::abs(errorStats.meanError), errorStats.meanAbsoluteError);
    
    // 百分比误差应该在合理范围内
    EXPECT_LT(errorStats.maxPercentError, 100.0);
}

// 测试数据归一化
TEST_F(DataProcessorTest, DataNormalization) {
    auto normalized = dataProcessor->normalizeData(linearData, 
        NormalizationMethod::MIN_MAX);
    
    // 验证归一化范围
    auto stats = dataProcessor->calculateStatistics(normalized);
    EXPECT_NEAR(stats.minHeight, 0.0, 0.001);
    EXPECT_NEAR(stats.maxHeight, 1.0, 0.001);
    
    // 测试Z-score归一化
    auto zScoreNormalized = dataProcessor->normalizeData(linearData,
        NormalizationMethod::Z_SCORE);
    
    auto zStats = dataProcessor->calculateStatistics(zScoreNormalized);
    EXPECT_NEAR(zStats.meanHeight, 0.0, 0.001);
    EXPECT_NEAR(zStats.stdDevHeight, 1.0, 0.001);
}

// 测试时间序列分析
TEST_F(DataProcessorTest, TimeSeriesAnalysis) {
    // 分析数据的时间特性
    auto timeSeries = dataProcessor->analyzeTimeSeries(linearData);
    
    // 验证采样率计算
    EXPECT_GT(timeSeries.samplingRate, 0.0);
    EXPECT_GT(timeSeries.totalDuration, 0.0);
    
    // 验证时间间隔统计
    EXPECT_GT(timeSeries.meanInterval, 0.0);
    EXPECT_GE(timeSeries.minInterval, 0.0);
    EXPECT_GE(timeSeries.maxInterval, timeSeries.minInterval);
}

// 测试数据导数计算
TEST_F(DataProcessorTest, DerivativeCalculation) {
    // 计算电容对高度的导数
    auto derivatives = dataProcessor->calculateDerivative(
        linearData, DataField::HEIGHT, DataField::CAPACITANCE);
    
    // 对于线性数据，导数应该是常数
    double expectedDerivative = 2.0; // 斜率
    for (const auto& deriv : derivatives) {
        EXPECT_NEAR(deriv.value, expectedDerivative, 0.1);
    }
}

// 测试积分计算
TEST_F(DataProcessorTest, IntegralCalculation) {
    // 计算曲线下面积
    double integral = dataProcessor->calculateIntegral(
        linearData, DataField::HEIGHT, DataField::CAPACITANCE);
    
    // 验证积分值合理
    EXPECT_GT(integral, 0.0);
    
    // 使用梯形法则验证
    double expectedIntegral = 0.0;
    for (size_t i = 1; i < linearData.size(); ++i) {
        double h1 = linearData[i-1].getSetHeight();
        double h2 = linearData[i].getSetHeight();
        double c1 = linearData[i-1].getSensorData().getMeasuredCapacitance();
        double c2 = linearData[i].getSensorData().getMeasuredCapacitance();
        expectedIntegral += 0.5 * (h2 - h1) * (c1 + c2);
    }
    
    EXPECT_NEAR(integral, expectedIntegral, 1.0);
}

// 测试数据聚类
TEST_F(DataProcessorTest, DataClustering) {
    // 创建具有明显聚类的数据
    std::vector<MeasurementData> clusterData;
    
    // 第一个聚类
    for (int i = 0; i < 10; i++) {
        SensorData sensor(20.0 + i, 20.0 + i, 150.0, 150.0, 25.0, 0.0, 100.0 + i);
        MeasurementData m(20.0 + i, 0.0, sensor);
        clusterData.push_back(m);
    }
    
    // 第二个聚类
    for (int i = 0; i < 10; i++) {
        SensorData sensor(60.0 + i, 60.0 + i, 150.0, 150.0, 25.0, 0.0, 200.0 + i);
        MeasurementData m(60.0 + i, 0.0, sensor);
        clusterData.push_back(m);
    }
    
    // 执行K-means聚类
    auto clusters = dataProcessor->performClustering(clusterData, 2,
        {DataField::HEIGHT, DataField::CAPACITANCE});
    
    EXPECT_EQ(clusters.size(), 2);
    
    // 每个聚类应该有大约10个点
    for (const auto& cluster : clusters) {
        EXPECT_NEAR(cluster.size(), 10, 2);
    }
}

// 辅助函数
double DataProcessorTest::calculateVariance(const std::vector<MeasurementData>& data) {
    if (data.empty()) return 0.0;
    
    double sum = 0.0;
    double sumSq = 0.0;
    
    for (const auto& m : data) {
        double val = m.getSetHeight();
        sum += val;
        sumSq += val * val;
    }
    
    double mean = sum / data.size();
    return (sumSq / data.size()) - (mean * mean);
} stats = dataProcessor->calculateStatistics(linearData);
    
    // 验证平均值
    EXPECT_NEAR(stats.meanHeight, 42.5, 0.1); // (20+25+...+65)/10
    EXPECT_NEAR(stats.meanAngle, 9.0, 0.1);   // (0+2+...+18)/10
    
    // 验证最小/最大值
    EXPECT_DOUBLE_EQ(stats.minHeight, 20.0);
    EXPECT_DOUBLE_EQ(stats.maxHeight, 65.0);
    EXPECT_DOUBLE_EQ(stats.minAngle, 0.0);
    EXPECT_DOUBLE_EQ(stats.maxAngle, 18.0);
    
    // 验证标准差
    EXPECT_GT(stats.stdDevHeight, 0.0);
    EXPECT_GT(stats.stdDevAngle, 0.0);
}

// 测试相关性分析
TEST_F(DataProcessorTest, CorrelationAnalysis) {
    // 线性数据应该有高相关性
    double correlation = dataProcessor->calculateCorrelation(
        linearData, DataField::HEIGHT, DataField::CAPACITANCE);
    
    EXPECT_NEAR(correlation, 1.0, 0.01); // 完美正相关
    
    // 测试负相关
    double negCorrelation = dataProcessor->calculateCorrelation(
        noisyData, DataField::HEIGHT, DataField::CAPACITANCE);
    
    EXPECT_LT(negCorrelation, 0.0); // 应该是负相关
}

// 测试线性回归
TEST_F(DataProcessorTest, LinearRegression) {
    auto regression = dataProcessor->performLinearRegression(
        linearData, DataField::HEIGHT, DataField::CAPACITANCE);
    
    // 验证斜率和截距
    EXPECT_NEAR(regression.slope, 2.0, 0.01);      // 每5mm高度增加10pF
    EXPECT_NEAR(regression.intercept, 60.0, 0.01); // 当高度=20时，电容=100
    
    // 验证R²值
    EXPECT_NEAR(regression.rSquared, 1.0, 0.001); // 完美拟合
    
    // 测试预测
    double predicted = dataProcessor->predict(regression, 30.0);
    EXPECT_NEAR(predicted, 120.0, 0.1); // 30*2 + 60 = 120
}

// 测试多项式拟合
TEST_F(DataProcessorTest, PolynomialFitting) {
    // 创建二次数据
    std::vector<MeasurementData> quadraticData;
    for (int i = -5; i <= 5; i++) {
        double x = i * 10.0;
        double y = 0.1 * x * x + 2.0 * x + 50.0; // y = 0.1x² + 2x + 50
        
        SensorData sensorData(x, x, 150.0, 150.0, 25.0, 0.0, y);
        MeasurementData measurement(x, 0.0, sensorData);
        quadraticData.push_back(measurement);
    }
    
    auto poly = dataProcessor->performPolynomialFitting(
        quadraticData, DataField::HEIGHT, DataField::CAPACITANCE, 2);
    
    // 验证系数
    EXPECT_EQ(poly.coefficients.size(), 3);
    EXPECT_NEAR(poly.coefficients[0], 50.0, 0.1);  // 常数项
    EXPECT_NEAR(poly.coefficients[1], 2.0, 0.1);   // 一次项
    EXPECT_NEAR(poly.coefficients[2], 0.1, 0.01);  // 二次项
    
    // 验证拟合质量
    EXPECT_GT(poly.rSquared, 0.99);
}

// 测试数据平滑
TEST_F(DataProcessorTest, DataSmoothing) {
    // 移动平均平滑
    auto smoothed = dataProcessor->smoothData(noisyData, 
        SmoothingMethod::MOVING_AVERAGE, 3);
    
    EXPECT_EQ(smoothed.size(), noisyData.size());
    
    // 平滑后的数据应该有更小的变化
    double originalVariance = calculateVariance(noisyData);
    double smoothedVariance = calculateVariance(smoothed);
    EXPECT_LT(smoothedVariance, originalVariance);
}

// 测试异常值检测
TEST_F(DataProcessorTest, OutlierDetection) {
    // 在数据中添加异常值
    auto dataWithOutliers = linearData;
    
    // 添加一个明显的异常值
    SensorData outlierSensor(10.0, 11.0, 150.0, 151.0, 25.0, 0.0, 500.0);
    MeasurementData outlier(25.0, 0.0, outlierSensor);
    dataWithOutliers.push_back(outlier);
    
    // 检测异常值
    auto outliers = dataProcessor->detectOutliers(dataWithOutliers, 
        DataField::CAPACITANCE, 2.0); // 2个标准差
    
    EXPECT_GE(outliers.size(), 1);
    EXPECT_EQ(outliers.back(), dataWithOutliers.size() - 1); // 最后一个是异常值
}

// 测试数据插值
TEST_F(DataProcessorTest, DataInterpolation) {
    // 创建稀疏数据
    std::vector<MeasurementData> sparseData;
    for (int i = 0; i < 5; i++) {
        double height = i * 20.0;
        double capacitance = 100.0 + i * 50.0;
        
        SensorData sensorData(height, height, 150.0, 150.0, 25.0, 0.0, capacitance);
        MeasurementData measurement(height, 0.0, sensorData);
        sparseData.push_back(measurement);
    }
    
    // 插值到更密集的网格
    auto interpolated = dataProcessor->interpolateData(sparseData, 
        DataField::HEIGHT, DataField::CAPACITANCE, 
        InterpolationMethod::LINEAR, 20);
    
    EXPECT_GT(interpolated.size(), sparseData.size());
    
    // 验证插值点
    // 在height=10时，电容应该是125（线性插值）
    auto it = std::find_if(interpolated.begin(), interpolated.end(),
        [](const MeasurementData& m) { 
            return std::abs(m.getSetHeight() - 10.0) < 0.1; 
        });
    
    if (it != interpolated.end()) {
        EXPECT_NEAR(it->getSensorData().getMeasuredCapacitance(), 125.0, 1.0);
    }
}

// 测试FFT分析
TEST_F(DataProcessorTest, FFTAnalysis) {
    // 创建包含多个频率成分的信号
    std::vector<MeasurementData> signalData;
    for (int i = 0; i < 100; i++) {
        double t = i * 0.1;
        // 组合信号：5Hz + 10Hz + 噪声
        double signal = 10.0 * std::sin(2 * M_PI * 5 * t) + 
                       5.0 * std::sin(2 * M_PI * 10 * t);
        
        SensorData sensorData(t, t, 150.0, 150.0, 25.0, 0.0, signal);
        MeasurementData measurement(t, 0.0, sensorData);
        signalData.push_back(measurement);
    }
    
    // 执行FFT
    auto spectrum = dataProcessor->performFFT(signalData, DataField::CAPACITANCE);
    
    // 应该检测到主要频率成分
    EXPECT_GT(spectrum.frequencies.size(), 0);
    EXPECT_GT(spectrum.magnitudes.size(), 0);
    
    // 找到峰值频率
    auto peaks = dataProcessor->findPeaks(spectrum, 0.5);
    EXPECT_GE(peaks.size(), 2); // 至少应该找到两个峰值（5Hz和10Hz）
}

// 测试数据分组分析
TEST_F(DataProcessorTest, GroupAnalysis) {
    // 按高度范围分组
    auto groups = dataProcessor->groupData(linearData, 
        [](const MeasurementData& m) {
            if (m.getSetHeight() < 30.0) return "Low";
            else if (m.getSetHeight() < 50.0) return "Medium";
            else return "High";
        });
    
    EXPECT_EQ(groups.size(), 3);
    
    // 对每组计算统计
    for (const auto& pair : groups) {
        auto stats = dataProcessor->calculateStatistics(pair.second);
        
        if (pair.first == "Low") {
            EXPECT_LT(stats.maxHeight, 30.0);
        } else if (pair.first == "High") {
            EXPECT_GE(stats.minHeight, 50.0);
        }
    }
}

