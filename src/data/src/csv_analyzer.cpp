// src/data/src/csv_analyzer.cpp
#include "../include/csv_analyzer.h"
#include "../../utils/include/logger.h"
#include "../../models/include/system_config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <set>

// 私有实现类
class CsvAnalyzer::Impl {
public:
    std::vector<MeasurementData> data;
    mutable DataStatistics cachedStats;
    mutable ErrorStatistics cachedErrorStats;
    mutable bool statsValid = false;
    mutable bool errorStatsValid = false;
};

CsvAnalyzer::CsvAnalyzer() : pImpl(std::make_unique<Impl>()) {
    LOG_INFO("CsvAnalyzer initialized");
}

CsvAnalyzer::~CsvAnalyzer() = default;

// ===== 数据加载与管理 =====

// 修复 loadCsvFile 方法中的 SensorData 设置部分
bool CsvAnalyzer::loadCsvFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open CSV file: " + filename);
        return false;
    }
    
    pImpl->data.clear();
    pImpl->statsValid = false;
    pImpl->errorStatsValid = false;
    
    std::string line;
    // 跳过标题行
    std::getline(file, line);
    
    // 读取数据行
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        // 解析CSV行
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string field;
        
        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }
        
        // 创建MeasurementData对象
        if (fields.size() >= 16) { // 确保有足够的字段
            try {
                // 解析基本字段
                double setHeight = std::stod(fields[1]);
                double setAngle = std::stod(fields[2]);
                
                // 创建SensorData
                SensorData sensorData;
                sensorData.setUpperSensors(std::stod(fields[4]), std::stod(fields[5]));
                sensorData.setLowerSensors(std::stod(fields[6]), std::stod(fields[7]));
                sensorData.setTemperature(std::stod(fields[8]));
                sensorData.setAngle(std::stod(fields[9]));  // 使用 setAngle 而不是 setMeasuredAngle
                sensorData.setCapacitance(std::stod(fields[10]));  // 使用 setCapacitance 而不是 setMeasuredCapacitance
                
                // 创建测量数据
                MeasurementData measurement(setHeight, setAngle, sensorData);
                pImpl->data.push_back(measurement);
                
            } catch (const std::exception& e) {
                LOG_WARNING("Failed to parse CSV line: " + std::string(e.what()));
            }
        }
    }
    
    file.close();
    LOG_INFO_F("Loaded %zu measurements from CSV", pImpl->data.size());
    return !pImpl->data.empty();
}

void CsvAnalyzer::setData(const std::vector<MeasurementData>& data) {
    pImpl->data = data;
    pImpl->statsValid = false;
    pImpl->errorStatsValid = false;
}

void CsvAnalyzer::clearData() {
    pImpl->data.clear();
    pImpl->statsValid = false;
    pImpl->errorStatsValid = false;
}

std::vector<MeasurementData> CsvAnalyzer::getData() const {
    return pImpl->data;
}

size_t CsvAnalyzer::getDataCount() const {
    return pImpl->data.size();
}

// ===== 数据过滤 =====

std::vector<MeasurementData> CsvAnalyzer::filterByHeight(double minHeight, double maxHeight) const {
    std::vector<MeasurementData> filtered;
    
    for (const auto& measurement : pImpl->data) {
        double height = measurement.getSetHeight();
        if (height >= minHeight && height <= maxHeight) {
            filtered.push_back(measurement);
        }
    }
    
    return filtered;
}

std::vector<MeasurementData> CsvAnalyzer::filterByAngle(double minAngle, double maxAngle) const {
    std::vector<MeasurementData> filtered;
    
    for (const auto& measurement : pImpl->data) {
        double angle = measurement.getSetAngle();
        if (angle >= minAngle && angle <= maxAngle) {
            filtered.push_back(measurement);
        }
    }
    
    return filtered;
}

// 修复 filterByTemperature 方法
std::vector<MeasurementData> CsvAnalyzer::filterByTemperature(double minTemp, double maxTemp) const {
    std::vector<MeasurementData> filtered;
    
    for (const auto& measurement : pImpl->data) {
        double temp = measurement.getSensorData().temperature;  // 直接访问公共成员
        if (temp >= minTemp && temp <= maxTemp) {
            filtered.push_back(measurement);
        }
    }
    
    return filtered;
}

std::vector<MeasurementData> CsvAnalyzer::getDataAtFixedAngle(double angle, double tolerance) const {
    std::vector<MeasurementData> filtered;
    
    for (const auto& measurement : pImpl->data) {
        double measurementAngle = measurement.getSetAngle();
        if (std::abs(measurementAngle - angle) <= tolerance) {
            filtered.push_back(measurement);
        }
    }
    
    // 按高度排序
    std::sort(filtered.begin(), filtered.end(),
              [](const MeasurementData& a, const MeasurementData& b) {
                  return a.getSetHeight() < b.getSetHeight();
              });
    
    return filtered;
}

std::vector<MeasurementData> CsvAnalyzer::getDataAtFixedHeight(double height, double tolerance) const {
    std::vector<MeasurementData> filtered;
    
    for (const auto& measurement : pImpl->data) {
        double measurementHeight = measurement.getSetHeight();
        if (std::abs(measurementHeight - height) <= tolerance) {
            filtered.push_back(measurement);
        }
    }
    
    // 按角度排序
    std::sort(filtered.begin(), filtered.end(),
              [](const MeasurementData& a, const MeasurementData& b) {
                  return a.getSetAngle() < b.getSetAngle();
              });
    
    return filtered;
}

// ===== 统计分析 =====

DataStatistics CsvAnalyzer::calculateStatistics() const {
    if (pImpl->statsValid) {
        return pImpl->cachedStats;
    }
    
    DataStatistics stats;
    if (pImpl->data.empty()) {
        return stats;
    }
    
    stats.dataCount = pImpl->data.size();
    
    // 提取各个字段的值
    std::vector<double> capacitances = extractMeasuredCapacitances(pImpl->data);
    std::vector<double> heights = extractHeights(pImpl->data);
    std::vector<double> angles = extractAngles(pImpl->data);
    std::vector<double> temperatures = extractTemperatures(pImpl->data);
    
    // 计算电容统计
    stats.meanCapacitance = calculateMean(capacitances);
    stats.stdDevCapacitance = calculateStdDev(capacitances, stats.meanCapacitance);
    auto [minC, maxC] = std::minmax_element(capacitances.begin(), capacitances.end());
    stats.minCapacitance = *minC;
    stats.maxCapacitance = *maxC;
    
    // 计算高度统计
    stats.meanHeight = calculateMean(heights);
    stats.stdDevHeight = calculateStdDev(heights, stats.meanHeight);
    auto [minH, maxH] = std::minmax_element(heights.begin(), heights.end());
    stats.minHeight = *minH;
    stats.maxHeight = *maxH;
    
    // 计算角度统计
    stats.meanAngle = calculateMean(angles);
    stats.stdDevAngle = calculateStdDev(angles, stats.meanAngle);
    auto [minA, maxA] = std::minmax_element(angles.begin(), angles.end());
    stats.minAngle = *minA;
    stats.maxAngle = *maxA;
    
    // 计算温度统计
    stats.meanTemperature = calculateMean(temperatures);
    stats.stdDevTemperature = calculateStdDev(temperatures, stats.meanTemperature);
    auto [minT, maxT] = std::minmax_element(temperatures.begin(), temperatures.end());
    stats.minTemperature = *minT;
    stats.maxTemperature = *maxT;
    
    pImpl->cachedStats = stats;
    pImpl->statsValid = true;
    
    return stats;
}

ErrorStatistics CsvAnalyzer::calculateErrorStatistics() const {
    if (pImpl->errorStatsValid) {
        return pImpl->cachedErrorStats;
    }
    
    ErrorStatistics errorStats;
    if (pImpl->data.empty()) {
        return errorStats;
    }
    
    std::vector<double> measured = extractMeasuredCapacitances(pImpl->data);
    std::vector<double> theoretical = extractTheoreticalCapacitances(pImpl->data);
    
    // 计算残差
    errorStats.residuals.resize(measured.size());
    double sumError = 0.0;
    double sumSquaredError = 0.0;
    double sumAbsError = 0.0;
    
    for (size_t i = 0; i < measured.size(); ++i) {
        double error = measured[i] - theoretical[i];
        errorStats.residuals[i] = error;
        
        sumError += error;
        sumSquaredError += error * error;
        sumAbsError += std::abs(error);
        
        if (i == 0 || error > errorStats.maxError) {
            errorStats.maxError = error;
        }
        if (i == 0 || error < errorStats.minError) {
            errorStats.minError = error;
        }
    }
    
    size_t n = measured.size();
    errorStats.meanError = sumError / n;
    errorStats.rmse = std::sqrt(sumSquaredError / n);
    errorStats.mae = sumAbsError / n;
    errorStats.r2Value = calculateR2(measured, theoretical);
    
    pImpl->cachedErrorStats = errorStats;
    pImpl->errorStatsValid = true;
    
    return errorStats;
}

// ===== 数据准备（用于图表） =====

// 修复 prepareDistanceCapacitanceData 方法
std::vector<DataPoint> CsvAnalyzer::prepareDistanceCapacitanceData(
    double fixedAngle, double fixedTemp,
    double angleTolerance, double tempTolerance) const {
    
    std::vector<DataPoint> points;
    
    // 过滤符合固定条件的数据
    for (const auto& measurement : pImpl->data) {
        double angle = measurement.getSetAngle();
        double temp = measurement.getSensorData().temperature;  // 直接访问公共成员
        
        if (std::abs(angle - fixedAngle) <= angleTolerance &&
            std::abs(temp - fixedTemp) <= tempTolerance) {
            
            DataPoint point;
            point.x = measurement.getSetHeight();
            point.y = measurement.getSensorData().capacitance;  // 直接访问公共成员
            point.color = temp; // 温度作为颜色
            points.push_back(point);
        }
    }
    
    // 按X值排序
    std::sort(points.begin(), points.end(),
              [](const DataPoint& a, const DataPoint& b) {
                  return a.x < b.x;
              });
    
    return points;
}

// 修复 prepareAngleCapacitanceData 方法
std::vector<DataPoint> CsvAnalyzer::prepareAngleCapacitanceData(
    double fixedHeight, double fixedTemp,
    double heightTolerance, double tempTolerance) const {
    
    std::vector<DataPoint> points;
    
    for (const auto& measurement : pImpl->data) {
        double height = measurement.getSetHeight();
        double temp = measurement.getSensorData().temperature;  // 直接访问公共成员
        
        if (std::abs(height - fixedHeight) <= heightTolerance &&
            std::abs(temp - fixedTemp) <= tempTolerance) {
            
            DataPoint point;
            point.x = measurement.getSetAngle();
            point.y = measurement.getSensorData().capacitance;  // 直接访问公共成员
            point.color = temp;
            points.push_back(point);
        }
    }
    
    std::sort(points.begin(), points.end(),
              [](const DataPoint& a, const DataPoint& b) {
                  return a.x < b.x;
              });
    
    return points;
}

// 修复 prepare3DData 方法
std::vector<DataPoint> CsvAnalyzer::prepare3DData() const {
    std::vector<DataPoint> points;
    
    for (const auto& measurement : pImpl->data) {
        DataPoint point;
        point.x = measurement.getSetHeight();  // 距离
        point.y = measurement.getSetAngle();   // 角度
        point.z = measurement.getSensorData().capacitance; // 电容 - 直接访问公共成员
        point.color = measurement.getSensorData().temperature; // 温度 - 直接访问公共成员
        points.push_back(point);
    }
    
    return points;
}

// 修复 prepareErrorAnalysisData 方法
std::vector<DataPoint> CsvAnalyzer::prepareErrorAnalysisData() const {
    std::vector<DataPoint> points;
    
    for (const auto& measurement : pImpl->data) {
        DataPoint point;
        point.x = measurement.getTheoreticalCapacitance();  // 理论值
        point.y = measurement.getSensorData().capacitance; // 测量值 - 直接访问公共成员
        points.push_back(point);
    }
    
    return points;
}

// ===== 辅助方法实现 =====

double CsvAnalyzer::calculateMean(const std::vector<double>& values) const {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double CsvAnalyzer::calculateStdDev(const std::vector<double>& values, double mean) const {
    if (values.size() < 2) return 0.0;
    
    double sum = 0.0;
    for (double val : values) {
        sum += (val - mean) * (val - mean);
    }
    
    return std::sqrt(sum / (values.size() - 1));
}

double CsvAnalyzer::calculateR2(const std::vector<double>& actual,
                                const std::vector<double>& predicted) const {
    if (actual.size() != predicted.size() || actual.empty()) {
        return 0.0;
    }
    
    double meanActual = calculateMean(actual);
    double ssTotal = 0.0;
    double ssResidual = 0.0;
    
    for (size_t i = 0; i < actual.size(); ++i) {
        ssTotal += (actual[i] - meanActual) * (actual[i] - meanActual);
        ssResidual += (actual[i] - predicted[i]) * (actual[i] - predicted[i]);
    }
    
    return (ssTotal > 0) ? 1.0 - (ssResidual / ssTotal) : 0.0;
}

// 修复 extractMeasuredCapacitances 方法
std::vector<double> CsvAnalyzer::extractMeasuredCapacitances(
    const std::vector<MeasurementData>& data) const {
    std::vector<double> values;
    values.reserve(data.size());
    
    for (const auto& measurement : data) {
        values.push_back(measurement.getSensorData().capacitance);  // 直接访问公共成员
    }
    
    return values;
}

// 添加其他缺失的提取方法实现
std::vector<double> CsvAnalyzer::extractHeights(const std::vector<MeasurementData>& data) const {
    std::vector<double> values;
    values.reserve(data.size());
    
    for (const auto& measurement : data) {
        values.push_back(measurement.getSetHeight());
    }
    
    return values;
}
// 其他提取方法类似...