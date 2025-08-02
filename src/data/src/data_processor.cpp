// src/data/src/data_processor.cpp
#include "../include/data_processor.h"
#include "../../utils/include/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

DataProcessor::DataProcessor() {
    workBuffer.reserve(10000); // 预分配工作缓冲区
    LOG_INFO("DataProcessor initialized");
}

DataProcessor::~DataProcessor() = default;

DataStatistics DataProcessor::calculateStatistics(const std::vector<MeasurementData>& data) {
    DataStatistics stats{};
    
    if (data.empty()) {
        return stats;
    }
    
    // 提取各字段值
    auto heights = extractFieldValues(data, DataField::HEIGHT);
    auto angles = extractFieldValues(data, DataField::ANGLE);
    auto capacitances = extractFieldValues(data, DataField::CAPACITANCE);
    
    // 计算均值
    stats.meanHeight = calculateMean(heights);
    stats.meanAngle = calculateMean(angles);
    stats.meanCapacitance = calculateMean(capacitances);
    
    // 计算标准差
    stats.stdDevHeight = calculateStdDev(heights, stats.meanHeight);
    stats.stdDevAngle = calculateStdDev(angles, stats.meanAngle);
    stats.stdDevCapacitance = calculateStdDev(capacitances, stats.meanCapacitance);
    
    // 计算最小/最大值
    auto [minH, maxH] = std::minmax_element(heights.begin(), heights.end());
    stats.minHeight = *minH;
    stats.maxHeight = *maxH;
    
    auto [minA, maxA] = std::minmax_element(angles.begin(), angles.end());
    stats.minAngle = *minA;
    stats.maxAngle = *maxA;
    
    auto [minC, maxC] = std::minmax_element(capacitances.begin(), capacitances.end());
    stats.minCapacitance = *minC;
    stats.maxCapacitance = *maxC;
    
    // 计算高级统计量
    stats.variance = calculateVariance(heights, stats.meanHeight);
    stats.skewness = calculateSkewness(heights, stats.meanHeight, stats.stdDevHeight);
    stats.kurtosis = calculateKurtosis(heights, stats.meanHeight, stats.stdDevHeight);
    
    return stats;
}

double DataProcessor::calculateCorrelation(const std::vector<MeasurementData>& data,
                                         DataField field1, DataField field2) {
    if (data.size() < 2) {
        return 0.0;
    }
    
    auto values1 = extractFieldValues(data, field1);
    auto values2 = extractFieldValues(data, field2);
    
    double mean1 = calculateMean(values1);
    double mean2 = calculateMean(values2);
    
    double covariance = 0.0;
    double variance1 = 0.0;
    double variance2 = 0.0;
    
    for (size_t i = 0; i < values1.size(); ++i) {
        double diff1 = values1[i] - mean1;
        double diff2 = values2[i] - mean2;
        
        covariance += diff1 * diff2;
        variance1 += diff1 * diff1;
        variance2 += diff2 * diff2;
    }
    
    if (variance1 * variance2 == 0) {
        return 0.0;
    }
    
    return covariance / std::sqrt(variance1 * variance2);
}

LinearRegression DataProcessor::performLinearRegression(const std::vector<MeasurementData>& data,
                                                      DataField xField, DataField yField) {
    LinearRegression result{};
    
    if (data.size() < 2) {
        LOG_WARNING("Insufficient data for linear regression");
        return result;
    }
    
    auto xValues = extractFieldValues(data, xField);
    auto yValues = extractFieldValues(data, yField);
    
    double n = static_cast<double>(data.size());
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
    
    for (size_t i = 0; i < xValues.size(); ++i) {
        sumX += xValues[i];
        sumY += yValues[i];
        sumXY += xValues[i] * yValues[i];
        sumX2 += xValues[i] * xValues[i];
    }
    
    // 计算斜率和截距
    double denominator = n * sumX2 - sumX * sumX;
    if (std::abs(denominator) < 1e-10) {
        LOG_WARNING("Near-zero denominator in linear regression");
        return result;
    }
    
    result.slope = (n * sumXY - sumX * sumY) / denominator;
    result.intercept = (sumY - result.slope * sumX) / n;
    
    // 计算R²和标准误差
    double ssTotal = 0.0, ssResidual = 0.0;
    double meanY = sumY / n;
    
    result.residuals.reserve(xValues.size());
    
    for (size_t i = 0; i < xValues.size(); ++i) {
        double predicted = result.slope * xValues[i] + result.intercept;
        double residual = yValues[i] - predicted;
        
        result.residuals.push_back(residual);
        ssResidual += residual * residual;
        ssTotal += (yValues[i] - meanY) * (yValues[i] - meanY);
    }
    
    result.rSquared = (ssTotal > 0) ? 1.0 - (ssResidual / ssTotal) : 0.0;
    result.standardError = std::sqrt(ssResidual / (n - 2));
    
    return result;
}

PolynomialFit DataProcessor::performPolynomialFitting(const std::vector<MeasurementData>& data,
                                                     DataField xField, DataField yField, int degree) {
    PolynomialFit result;
    result.degree = degree;
    
    if (data.size() < static_cast<size_t>(degree + 1)) {
        LOG_WARNING("Insufficient data for polynomial fitting");
        return result;
    }
    
    auto xValues = extractFieldValues(data, xField);
    auto yValues = extractFieldValues(data, yField);
    
    // 构建范德蒙矩阵
    int n = data.size();
    int m = degree + 1;
    
    std::vector<std::vector<double>> vandermonde(n, std::vector<double>(m));
    for (int i = 0; i < n; ++i) {
        vandermonde[i][0] = 1.0;
        for (int j = 1; j < m; ++j) {
            vandermonde[i][j] = vandermonde[i][j-1] * xValues[i];
        }
    }
    
    // 使用最小二乘法求解
    // 这里使用简化的正规方程方法，实际应用中可能需要更稳定的算法
    std::vector<std::vector<double>> ata(m, std::vector<double>(m, 0.0));
    std::vector<double> atb(m, 0.0);
    
    // 计算 A^T * A 和 A^T * b
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < m; ++j) {
            for (int k = 0; k < n; ++k) {
                ata[i][j] += vandermonde[k][i] * vandermonde[k][j];
            }
        }
        
        for (int k = 0; k < n; ++k) {
            atb[i] += vandermonde[k][i] * yValues[k];
        }
    }
    
    // 高斯消元法求解
    result.coefficients = solveLinearSystem(ata, atb);
    
    // 计算拟合质量
    double ssTotal = 0.0, ssResidual = 0.0;
    double meanY = calculateMean(yValues);
    
    for (size_t i = 0; i < xValues.size(); ++i) {
        double predicted = predict(result, xValues[i]);
        double residual = yValues[i] - predicted;
        
        ssResidual += residual * residual;
        ssTotal += (yValues[i] - meanY) * (yValues[i] - meanY);
    }
    
    result.rSquared = (ssTotal > 0) ? 1.0 - (ssResidual / ssTotal) : 0.0;
    result.rmse = std::sqrt(ssResidual / n);
    
    return result;
}

double DataProcessor::predict(const LinearRegression& model, double x) {
    return model.slope * x + model.intercept;
}

double DataProcessor::predict(const PolynomialFit& model, double x) {
    double result = 0.0;
    double xPower = 1.0;
    
    for (size_t i = 0; i < model.coefficients.size(); ++i) {
        result += model.coefficients[i] * xPower;
        xPower *= x;
    }
    
    return result;
}

std::vector<MeasurementData> DataProcessor::smoothData(const std::vector<MeasurementData>& data,
                                                      SmoothingMethod method, int windowSize) {
    if (data.empty() || windowSize <= 0) {
        return data;
    }
    
    std::vector<MeasurementData> smoothed = data;
    
    // 对每个数值字段进行平滑
    std::vector<DataField> fieldsToSmooth = {
        DataField::HEIGHT, DataField::ANGLE, DataField::CAPACITANCE,
        DataField::UPPER_SENSOR_1, DataField::UPPER_SENSOR_2,
        DataField::LOWER_SENSOR_1, DataField::LOWER_SENSOR_2
    };
    
    for (auto field : fieldsToSmooth) {
        auto values = extractFieldValues(data, field);
        std::vector<double> smoothedValues;
        
        switch (method) {
            case SmoothingMethod::MOVING_AVERAGE:
                smoothedValues = movingAverage(values, windowSize);
                break;
                
            case SmoothingMethod::GAUSSIAN:
                smoothedValues = gaussianSmooth(values, windowSize, windowSize / 3.0);
                break;
                
            case SmoothingMethod::MEDIAN:
                smoothedValues = medianFilter(values, windowSize);
                break;
                
            case SmoothingMethod::SAVITZKY_GOLAY:
                // Savitzky-Golay需要更复杂的实现
                smoothedValues = movingAverage(values, windowSize);
                break;
        }
        
        // 将平滑后的值写回
        for (size_t i = 0; i < smoothed.size(); ++i) {
            setFieldValue(smoothed[i], field, smoothedValues[i]);
        }
    }
    
    return smoothed;
}

std::vector<size_t> DataProcessor::detectOutliers(const std::vector<MeasurementData>& data,
                                                 DataField field, double threshold) {
    std::vector<size_t> outliers;
    
    if (data.size() < 3) {
        return outliers;
    }
    
    auto values = extractFieldValues(data, field);
    double mean = calculateMean(values);
    double stdDev = calculateStdDev(values, mean);
    
    // Z-score方法检测异常值
    for (size_t i = 0; i < values.size(); ++i) {
        double zScore = std::abs((values[i] - mean) / stdDev);
        if (zScore > threshold) {
            outliers.push_back(i);
        }
    }
    
    return outliers;
}

// 继续 data_processor.cpp

std::vector<MeasurementData> DataProcessor::interpolateData(const std::vector<MeasurementData>& data,
                                                           DataField xField, DataField yField,
                                                           InterpolationMethod method,
                                                           int numPoints) {
    if (data.size() < 2 || numPoints <= 0) {
        return data;
    }
    
    auto xValues = extractFieldValues(data, xField);
    auto yValues = extractFieldValues(data, yField);
    
    // 找到x的范围
    auto [minX, maxX] = std::minmax_element(xValues.begin(), xValues.end());
    double xMin = *minX;
    double xMax = *maxX;
    double xStep = (xMax - xMin) / (numPoints - 1);
    
    std::vector<MeasurementData> interpolated;
    
    for (int i = 0; i < numPoints; ++i) {
        double x = xMin + i * xStep;
        double y = 0.0;
        
        switch (method) {
            case InterpolationMethod::LINEAR:
                // 找到相邻的两个点
                for (size_t j = 1; j < xValues.size(); ++j) {
                    if (x >= xValues[j-1] && x <= xValues[j]) {
                        y = linearInterpolate(xValues[j-1], yValues[j-1], 
                                            xValues[j], yValues[j], x);
                        break;
                    }
                }
                break;
                
            case InterpolationMethod::CUBIC:
            case InterpolationMethod::SPLINE:
                y = cubicInterpolate(xValues, yValues, x);
                break;
        }
        
        // 创建新的测量数据点
        MeasurementData newPoint = data[0]; // 复制模板
        setFieldValue(newPoint, xField, x);
        setFieldValue(newPoint, yField, y);
        interpolated.push_back(newPoint);
    }
    
    return interpolated;
}

FFTResult DataProcessor::performFFT(const std::vector<MeasurementData>& data, DataField field) {
    FFTResult result;
    
    if (data.size() < 2) {
        return result;
    }
    
    auto values = extractFieldValues(data, field);
    
    // 计算采样率
    auto timestamps = extractFieldValues(data, DataField::TIMESTAMP);
    double totalTime = (timestamps.back() - timestamps.front()) / 1000.0; // 转换为秒
    result.samplingRate = (data.size() - 1) / totalTime;
    
    // 准备FFT输入（扩展到2的幂）
    size_t n = 1;
    while (n < values.size()) n *= 2;
    
    std::vector<std::complex<double>> fftData(n);
    for (size_t i = 0; i < values.size(); ++i) {
        fftData[i] = std::complex<double>(values[i], 0.0);
    }
    
    // 执行FFT
    auto spectrum = fft(fftData);
    
    // 计算频率和幅度
    result.frequencies.resize(n/2);
    result.magnitudes.resize(n/2);
    result.phases.resize(n/2);
    
    for (size_t i = 0; i < n/2; ++i) {
        result.frequencies[i] = i * result.samplingRate / n;
        result.magnitudes[i] = std::abs(spectrum[i]) * 2.0 / n;
        result.phases[i] = std::arg(spectrum[i]);
    }
    
    // 找到主频率
    auto maxIt = std::max_element(result.magnitudes.begin() + 1, result.magnitudes.end());
    if (maxIt != result.magnitudes.end()) {
        size_t maxIdx = std::distance(result.magnitudes.begin(), maxIt);
        result.dominantFrequency = result.frequencies[maxIdx];
    }
    
    return result;
}

std::vector<size_t> DataProcessor::findPeaks(const FFTResult& fft, double threshold) {
    std::vector<size_t> peaks;
    
    if (fft.magnitudes.size() < 3) {
        return peaks;
    }
    
    double maxMagnitude = *std::max_element(fft.magnitudes.begin(), fft.magnitudes.end());
    double peakThreshold = maxMagnitude * threshold;
    
    // 查找局部最大值
    for (size_t i = 1; i < fft.magnitudes.size() - 1; ++i) {
        if (fft.magnitudes[i] > peakThreshold &&
            fft.magnitudes[i] > fft.magnitudes[i-1] &&
            fft.magnitudes[i] > fft.magnitudes[i+1]) {
            peaks.push_back(i);
        }
    }
    
    return peaks;
}

TrendAnalysis DataProcessor::analyzeTrend(const std::vector<MeasurementData>& data, DataField field) {
    TrendAnalysis result;
    
    if (data.size() < 3) {
        result.direction = TrendDirection::STABLE;
        result.strength = 0.0;
        return result;
    }
    
    auto values = extractFieldValues(data, field);
    
    // 执行线性回归
    std::vector<double> indices(values.size());
    std::iota(indices.begin(), indices.end(), 0.0);
    
    // 计算线性趋势
    double n = values.size();
    double sumX = n * (n - 1) / 2;
    double sumX2 = n * (n - 1) * (2 * n - 1) / 6;
    double sumY = std::accumulate(values.begin(), values.end(), 0.0);
    double sumXY = 0.0;
    
    for (size_t i = 0; i < values.size(); ++i) {
        sumXY += i * values[i];
    }
    
    double slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    double intercept = (sumY - slope * sumX) / n;
    
    // 确定趋势方向
    if (std::abs(slope) < 0.001) {
        result.direction = TrendDirection::STABLE;
    } else if (slope > 0) {
        result.direction = TrendDirection::INCREASING;
    } else {
        result.direction = TrendDirection::DECREASING;
    }
    
    // 计算趋势强度（R²）
    double ssTotal = 0.0, ssResidual = 0.0;
    double meanY = sumY / n;
    result.trendLine.resize(values.size());
    
    for (size_t i = 0; i < values.size(); ++i) {
        double predicted = slope * i + intercept;
        result.trendLine[i] = predicted;
        
        ssResidual += std::pow(values[i] - predicted, 2);
        ssTotal += std::pow(values[i] - meanY, 2);
    }
    
    result.strength = (ssTotal > 0) ? std::sqrt(1.0 - ssResidual / ssTotal) : 0.0;
    result.changeRate = slope;
    
    // 计算加速度（二阶导数）
    if (values.size() > 3) {
        std::vector<double> firstDerivative(values.size() - 1);
        for (size_t i = 1; i < values.size(); ++i) {
            firstDerivative[i-1] = values[i] - values[i-1];
        }
        
        double sumDerivative = 0.0;
        for (size_t i = 1; i < firstDerivative.size(); ++i) {
            sumDerivative += firstDerivative[i] - firstDerivative[i-1];
        }
        result.acceleration = sumDerivative / (firstDerivative.size() - 1);
    }
    
    return result;
}

ChartData2D DataProcessor::prepareScatterPlotData(const std::vector<MeasurementData>& data,
                                                 DataField xField, DataField yField) {
    ChartData2D chartData;
    
    chartData.xValues = extractFieldValues(data, xField);
    chartData.yValues = extractFieldValues(data, yField);
    chartData.xLabel = getFieldName(xField) + " (" + getFieldUnit(xField) + ")";
    chartData.yLabel = getFieldName(yField) + " (" + getFieldUnit(yField) + ")";
    chartData.title = chartData.yLabel + " vs " + chartData.xLabel;
    
    return chartData;
}

ChartData3D DataProcessor::prepare3DSurfaceData(const std::vector<MeasurementData>& data,
                                              DataField xField, DataField yField, DataField zField) {
    ChartData3D chartData;
    
    auto xValues = extractFieldValues(data, xField);
    auto yValues = extractFieldValues(data, yField);
    auto zValues = extractFieldValues(data, zField);
    
    // 创建网格
    std::set<double> uniqueX(xValues.begin(), xValues.end());
    std::set<double> uniqueY(yValues.begin(), yValues.end());
    
    chartData.xGrid.assign(uniqueX.begin(), uniqueX.end());
    chartData.yGrid.assign(uniqueY.begin(), uniqueY.end());
    
    // 初始化Z值矩阵
    chartData.zValues.resize(chartData.yGrid.size(), 
                            std::vector<double>(chartData.xGrid.size(), 0.0));
    
    // 填充Z值（这里使用最近邻插值）
    for (size_t i = 0; i < data.size(); ++i) {
        auto xIt = std::find(chartData.xGrid.begin(), chartData.xGrid.end(), xValues[i]);
        auto yIt = std::find(chartData.yGrid.begin(), chartData.yGrid.end(), yValues[i]);
        
        if (xIt != chartData.xGrid.end() && yIt != chartData.yGrid.end()) {
            size_t xIdx = std::distance(chartData.xGrid.begin(), xIt);
            size_t yIdx = std::distance(chartData.yGrid.begin(), yIt);
            chartData.zValues[yIdx][xIdx] = zValues[i];
        }
    }
    
    chartData.xLabel = getFieldName(xField) + " (" + getFieldUnit(xField) + ")";
    chartData.yLabel = getFieldName(yField) + " (" + getFieldUnit(yField) + ")";
    chartData.zLabel = getFieldName(zField) + " (" + getFieldUnit(zField) + ")";
    chartData.title = "3D Surface Plot";
    
    return chartData;
}

ErrorAnalysis DataProcessor::analyzeError(const std::vector<double>& theoretical,
                                        const std::vector<double>& measured) {
    ErrorAnalysis result;
    
    if (theoretical.size() != measured.size() || theoretical.empty()) {
        LOG_WARNING("Invalid data for error analysis");
        return result;
    }
    
    result.errors.resize(theoretical.size());
    double sumError = 0.0;
    double sumAbsError = 0.0;
    double sumSquaredError = 0.0;
    result.maxError = 0.0;
    result.minError = std::numeric_limits<double>::max();
    result.maxPercentError = 0.0;
    
    for (size_t i = 0; i < theoretical.size(); ++i) {
        double error = measured[i] - theoretical[i];
        result.errors[i] = error;
        
        sumError += error;
        sumAbsError += std::abs(error);
        sumSquaredError += error * error;
        
        result.maxError = std::max(result.maxError, error);
        result.minError = std::min(result.minError, error);
        
        if (std::abs(theoretical[i]) > 1e-10) {
            double percentError = std::abs(error / theoretical[i]) * 100.0;
            result.maxPercentError = std::max(result.maxPercentError, percentError);
        }
    }
    
    size_t n = theoretical.size();
    result.meanError = sumError / n;
    result.meanAbsoluteError = sumAbsError / n;
    result.rootMeanSquareError = std::sqrt(sumSquaredError / n);
    result.standardDeviation = calculateStdDev(result.errors, result.meanError);
    
    return result;
}

// 私有辅助方法实现

double DataProcessor::getFieldValue(const MeasurementData& data, DataField field) const {
    switch (field) {
        case DataField::HEIGHT:
            return data.getSetHeight();
        case DataField::ANGLE:
            return data.getSetAngle();
        case DataField::CAPACITANCE:
            return data.getSensorData().getMeasuredCapacitance();
        case DataField::TEMPERATURE:
            return data.getSensorData().getTemperature();
        case DataField::UPPER_SENSOR_1:
            return data.getSensorData().getUpperSensor1();
        case DataField::UPPER_SENSOR_2:
            return data.getSensorData().getUpperSensor2();
        case DataField::LOWER_SENSOR_1:
            return data.getSensorData().getLowerSensor1();
        case DataField::LOWER_SENSOR_2:
            return data.getSensorData().getLowerSensor2();
        case DataField::TIMESTAMP:
            return static_cast<double>(data.getTimestamp());
        default:
            return 0.0;
    }
}

std::string DataProcessor::getFieldName(DataField field) const {
    switch (field) {
        case DataField::HEIGHT: return "Height";
        case DataField::ANGLE: return "Angle";
        case DataField::CAPACITANCE: return "Capacitance";
        case DataField::TEMPERATURE: return "Temperature";
        case DataField::UPPER_SENSOR_1: return "Upper Sensor 1";
        case DataField::UPPER_SENSOR_2: return "Upper Sensor 2";
        case DataField::LOWER_SENSOR_1: return "Lower Sensor 1";
        case DataField::LOWER_SENSOR_2: return "Lower Sensor 2";
        case DataField::TIMESTAMP: return "Timestamp";
        default: return "Unknown";
    }
}

std::string DataProcessor::getFieldUnit(DataField field) const {
    switch (field) {
        case DataField::HEIGHT:
        case DataField::UPPER_SENSOR_1:
        case DataField::UPPER_SENSOR_2:
        case DataField::LOWER_SENSOR_1:
        case DataField::LOWER_SENSOR_2:
            return "mm";
        case DataField::ANGLE:
            return "deg";
        case DataField::CAPACITANCE:
            return "pF";
        case DataField::TEMPERATURE:
            return "°C";
        case DataField::TIMESTAMP:
            return "ms";
        default:
            return "";
    }
}

double DataProcessor::calculateMean(const std::vector<double>& values) const {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double DataProcessor::calculateStdDev(const std::vector<double>& values, double mean) const {
    if (values.size() < 2) return 0.0;
    
    double sum = 0.0;
    for (double val : values) {
        sum += (val - mean) * (val - mean);
    }
    
    return std::sqrt(sum / (values.size() - 1));
}

std::vector<double> DataProcessor::movingAverage(const std::vector<double>& data, int windowSize) const {
    std::vector<double> result(data.size());
    int halfWindow = windowSize / 2;
    
    for (size_t i = 0; i < data.size(); ++i) {
        int start = std::max(0, static_cast<int>(i) - halfWindow);
        int end = std::min(static_cast<int>(data.size()), static_cast<int>(i) + halfWindow + 1);
        
        double sum = 0.0;
        for (int j = start; j < end; ++j) {
            sum += data[j];
        }
        
        result[i] = sum / (end - start);
    }
    
    return result;
}

// 注意：完整的实现需要包含更多辅助方法，如FFT实现、高斯平滑、聚类算法等
// 这些复杂算法通常会使用专门的数学库（如FFTW、Eigen等）