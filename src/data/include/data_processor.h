// src/data/include/data_processor.h
#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <vector>
#include <map>
#include <string>
#include <functional>
#include <complex>
#include "../../models/include/measurement_data.h"
#include "../../models/include/data_statistics.h"

/**
 * @brief 数据字段枚举
 */
enum class DataField {
    HEIGHT,
    ANGLE,
    CAPACITANCE,
    TEMPERATURE,
    UPPER_SENSOR_1,
    UPPER_SENSOR_2,
    LOWER_SENSOR_1,
    LOWER_SENSOR_2,
    TIMESTAMP
};

/**
 * @brief 平滑方法枚举
 */
enum class SmoothingMethod {
    MOVING_AVERAGE,
    GAUSSIAN,
    MEDIAN,
    SAVITZKY_GOLAY
};

/**
 * @brief 插值方法枚举
 */
enum class InterpolationMethod {
    LINEAR,
    CUBIC,
    SPLINE
};

/**
 * @brief 归一化方法枚举
 */
enum class NormalizationMethod {
    MIN_MAX,
    Z_SCORE,
    DECIMAL_SCALING
};

/**
 * @brief 趋势方向枚举
 */
enum class TrendDirection {
    INCREASING,
    DECREASING,
    STABLE,
    OSCILLATING
};

/**
 * @brief 线性回归结果
 */
struct LinearRegression {
    double slope;
    double intercept;
    double rSquared;
    double standardError;
    std::vector<double> residuals;
};

/**
 * @brief 多项式拟合结果
 */
struct PolynomialFit {
    std::vector<double> coefficients; // 从低阶到高阶
    int degree;
    double rSquared;
    double rmse;
};

/**
 * @brief FFT结果
 */
struct FFTResult {
    std::vector<double> frequencies;
    std::vector<double> magnitudes;
    std::vector<double> phases;
    double dominantFrequency;
    double samplingRate;
};

/**
 * @brief 趋势分析结果
 */
struct TrendAnalysis {
    TrendDirection direction;
    double strength; // 0-1
    double changeRate;
    double acceleration;
    std::vector<double> trendLine;
};

/**
 * @brief 误差分析结果
 */
struct ErrorAnalysis {
    double meanError;
    double meanAbsoluteError;
    double rootMeanSquareError;
    double maxError;
    double minError;
    double standardDeviation;
    double maxPercentError;
    std::vector<double> errors;
};

/**
 * @brief 时间序列分析结果
 */
struct TimeSeriesAnalysis {
    double samplingRate;
    double meanInterval;
    double minInterval;
    double maxInterval;
    double totalDuration;
    bool isUniform;
    std::vector<double> intervals;
};

/**
 * @brief 导数点
 */
struct DerivativePoint {
    double x;
    double value;
    double secondDerivative;
};

/**
 * @brief 2D图表数据
 */
struct ChartData2D {
    std::vector<double> xValues;
    std::vector<double> yValues;
    std::string xLabel;
    std::string yLabel;
    std::string title;
};

/**
 * @brief 3D图表数据
 */
struct ChartData3D {
    std::vector<double> xGrid;
    std::vector<double> yGrid;
    std::vector<std::vector<double>> zValues;
    std::string xLabel;
    std::string yLabel;
    std::string zLabel;
    std::string title;
};

/**
 * @brief 数据处理器类
 * 
 * 提供高级数据分析功能：
 * - 统计分析
 * - 回归和拟合
 * - 信号处理
 * - 趋势分析
 * - 图表数据准备
 */
class DataProcessor {
    public:
        /**
         * @brief 构造函数
         */
        DataProcessor();
        
        /**
         * @brief 析构函数
         */
        ~DataProcessor();
        
        // 统计分析
        DataStatistics calculateStatistics(const std::vector<MeasurementData>& data);
        double calculateCorrelation(const std::vector<MeasurementData>& data,
                                   DataField field1, DataField field2);
        
        // 回归分析
        LinearRegression performLinearRegression(const std::vector<MeasurementData>& data,
                                               DataField xField, DataField yField);
        PolynomialFit performPolynomialFitting(const std::vector<MeasurementData>& data,
                                              DataField xField, DataField yField, int degree);
        double predict(const LinearRegression& model, double x);
        double predict(const PolynomialFit& model, double x);
        
        // 数据平滑
        std::vector<MeasurementData> smoothData(const std::vector<MeasurementData>& data,
                                               SmoothingMethod method, int windowSize);
        
        // 异常值检测
        std::vector<size_t> detectOutliers(const std::vector<MeasurementData>& data,
                                           DataField field, double threshold = 2.0);
        
        // 数据插值
        std::vector<MeasurementData> interpolateData(const std::vector<MeasurementData>& data,
                                                    DataField xField, DataField yField,
                                                    InterpolationMethod method,
                                                    int numPoints);
        
        // FFT分析
        FFTResult performFFT(const std::vector<MeasurementData>& data, DataField field);
        std::vector<size_t> findPeaks(const FFTResult& fft, double threshold);
        
        // 数据分组
        std::map<std::string, std::vector<MeasurementData>> groupData(
            const std::vector<MeasurementData>& data,
            std::function<std::string(const MeasurementData&)> groupingFunction);
        
        // 趋势分析
        TrendAnalysis analyzeTrend(const std::vector<MeasurementData>& data, DataField field);
        
        // 图表数据准备
        ChartData2D prepareScatterPlotData(const std::vector<MeasurementData>& data,
                                           DataField xField, DataField yField);
        ChartData3D prepare3DSurfaceData(const std::vector<MeasurementData>& data,
                                         DataField xField, DataField yField, DataField zField);
        
        // 误差分析
        ErrorAnalysis analyzeError(const std::vector<double>& theoretical,
                                  const std::vector<double>& measured);
        
        // 数据归一化
        std::vector<MeasurementData> normalizeData(const std::vector<MeasurementData>& data,
                                                  NormalizationMethod method);
        
        // 时间序列分析
        TimeSeriesAnalysis analyzeTimeSeries(const std::vector<MeasurementData>& data);
        
        // 导数和积分
        std::vector<DerivativePoint> calculateDerivative(const std::vector<MeasurementData>& data,
                                                        DataField xField, DataField yField);
        double calculateIntegral(const std::vector<MeasurementData>& data,
                               DataField xField, DataField yField);
        
        // 聚类分析
        std::vector<std::vector<MeasurementData>> performClustering(
            const std::vector<MeasurementData>& data,
            int numClusters,
            const std::vector<DataField>& features);
        
        // 实用方法
        std::string getFieldName(DataField field) const;
        std::string getFieldUnit(DataField field) const;
        
    private:
        // 辅助方法
        double getFieldValue(const MeasurementData& data, DataField field) const;
        void setFieldValue(MeasurementData& data, DataField field, double value) const;
        std::vector<double> extractFieldValues(const std::vector<MeasurementData>& data,
                                             DataField field) const;
        
        // 统计计算
        double calculateMean(const std::vector<double>& values) const;
        double calculateStdDev(const std::vector<double>& values, double mean) const;
        double calculateVariance(const std::vector<double>& values, double mean) const;
        double calculateSkewness(const std::vector<double>& values, double mean, double stdDev) const;
        double calculateKurtosis(const std::vector<double>& values, double mean, double stdDev) const;
        
        // 数学工具
        std::vector<double> movingAverage(const std::vector<double>& data, int windowSize) const;
        std::vector<double> gaussianSmooth(const std::vector<double>& data, int windowSize, double sigma) const;
        std::vector<double> medianFilter(const std::vector<double>& data, int windowSize) const;
        
        // 线性代数
        std::vector<double> gaussianElimination(std::vector<std::vector<double>>& a, 
                                              std::vector<double>& b) const;
        
        // FFT实现
        std::vector<std::complex<double>> fft(const std::vector<std::complex<double>>& data) const;
        std::vector<std::complex<double>> ifft(const std::vector<std::complex<double>>& data) const;
        
        // 插值实现
        double linearInterpolate(double x0, double y0, double x1, double y1, double x) const;
        double cubicInterpolate(const std::vector<double>& x, const std::vector<double>& y, double xi) const;
        
        // K-means聚类
        std::vector<std::vector<size_t>> kMeansClustering(const std::vector<std::vector<double>>& data,
                                                          int k, int maxIterations = 100) const;
        
        // 成员变量
        mutable std::vector<double> workBuffer; // 工作缓冲区
    };
    
#endif // DATA_PROCESSOR_H