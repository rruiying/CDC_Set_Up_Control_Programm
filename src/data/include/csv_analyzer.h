// src/data/include/csv_analyzer.h
#ifndef CSV_ANALYZER_H
#define CSV_ANALYZER_H

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include "../../models/include/measurement_data.h"

// 统计数据结构
struct DataStatistics {
    size_t dataCount = 0;
    
    // 电容统计
    double meanCapacitance = 0.0;
    double stdDevCapacitance = 0.0;
    double minCapacitance = 0.0;
    double maxCapacitance = 0.0;
    
    // 高度统计
    double meanHeight = 0.0;
    double stdDevHeight = 0.0;
    double minHeight = 0.0;
    double maxHeight = 0.0;
    
    // 角度统计
    double meanAngle = 0.0;
    double stdDevAngle = 0.0;
    double minAngle = 0.0;
    double maxAngle = 0.0;
    
    // 温度统计
    double meanTemperature = 0.0;
    double stdDevTemperature = 0.0;
    double minTemperature = 0.0;
    double maxTemperature = 0.0;
};

// 误差统计结构
struct ErrorStatistics {
    double rmse = 0.0;          // 均方根误差
    double meanError = 0.0;      // 平均误差
    double maxError = 0.0;       // 最大误差
    double minError = 0.0;       // 最小误差
    double r2Value = 0.0;        // 决定系数
    double mae = 0.0;            // 平均绝对误差
    std::vector<double> residuals; // 残差数组
};

// 回归分析结果
struct RegressionResult {
    double slope = 0.0;          // 斜率
    double intercept = 0.0;      // 截距
    double r2 = 0.0;             // R²值
    double standardError = 0.0;  // 标准误差
    std::vector<double> fittedValues; // 拟合值
};

// 数据点结构（用于图表）
struct DataPoint {
    double x;
    double y;
    double z;       // 用于3D图表
    double color;   // 用于颜色映射（如温度）
    
    DataPoint(double x = 0, double y = 0, double z = 0, double color = 0)
        : x(x), y(y), z(z), color(color) {}
};

// 过滤条件
struct FilterConditions {
    bool enableHeightFilter = false;
    double minHeight = 0.0;
    double maxHeight = 100.0;
    
    bool enableAngleFilter = false;
    double minAngle = -90.0;
    double maxAngle = 90.0;
    
    bool enableTemperatureFilter = false;
    double minTemperature = -40.0;
    double maxTemperature = 85.0;
    
    bool enableCapacitanceFilter = false;
    double minCapacitance = 0.0;
    double maxCapacitance = 1000.0;
};

class CsvAnalyzer {
public:
    CsvAnalyzer();
    ~CsvAnalyzer();
    
    // ===== 数据加载与管理 =====
    
    // 从CSV文件加载数据
    bool loadCsvFile(const std::string& filename);
    
    // 设置数据（从内存）
    void setData(const std::vector<MeasurementData>& data);
    
    // 清空数据
    void clearData();
    
    // 获取数据
    std::vector<MeasurementData> getData() const;
    
    // 获取数据数量
    size_t getDataCount() const;
    
    // ===== 数据过滤 =====
    
    // 按高度范围过滤
    std::vector<MeasurementData> filterByHeight(double minHeight, double maxHeight) const;
    
    // 按角度范围过滤
    std::vector<MeasurementData> filterByAngle(double minAngle, double maxAngle) const;
    
    // 按温度范围过滤
    std::vector<MeasurementData> filterByTemperature(double minTemp, double maxTemp) const;
    
    // 组合过滤
    std::vector<MeasurementData> filterData(const FilterConditions& conditions) const;
    
    // 获取固定条件下的数据（用于2D图表）
    std::vector<MeasurementData> getDataAtFixedAngle(double angle, double tolerance = 1.0) const;
    std::vector<MeasurementData> getDataAtFixedHeight(double height, double tolerance = 1.0) const;
    std::vector<MeasurementData> getDataAtFixedTemperature(double temp, double tolerance = 0.5) const;
    
    // ===== 统计分析 =====
    
    // 计算基本统计信息
    DataStatistics calculateStatistics() const;
    DataStatistics calculateStatistics(const std::vector<MeasurementData>& data) const;
    
    // 计算误差统计
    ErrorStatistics calculateErrorStatistics() const;
    ErrorStatistics calculateErrorStatistics(const std::vector<MeasurementData>& data) const;
    
    // ===== 回归分析 =====
    
    // 线性回归分析
    RegressionResult performLinearRegression(
        const std::vector<DataPoint>& points) const;
    
    // 多项式回归（degree = 多项式阶数）
    RegressionResult performPolynomialRegression(
        const std::vector<DataPoint>& points, int degree) const;
    
    // ===== 数据准备（用于图表） =====
    
    // 准备距离-电容数据（固定角度和温度）
    std::vector<DataPoint> prepareDistanceCapacitanceData(
        double fixedAngle = 0.0, double fixedTemp = 25.0,
        double angleTolerance = 2.0, double tempTolerance = 1.0) const;
    
    // 准备角度-电容数据（固定高度和温度）
    std::vector<DataPoint> prepareAngleCapacitanceData(
        double fixedHeight = 50.0, double fixedTemp = 25.0,
        double heightTolerance = 2.0, double tempTolerance = 1.0) const;
    
    // 准备温度-电容数据（固定高度和角度）
    std::vector<DataPoint> prepareTemperatureCapacitanceData(
        double fixedHeight = 50.0, double fixedAngle = 0.0,
        double heightTolerance = 2.0, double angleTolerance = 2.0) const;
    
    // 准备3D数据（距离-角度-电容，温度作为颜色）
    std::vector<DataPoint> prepare3DData() const;
    
    // 准备误差分析数据（理论值vs实测值）
    std::vector<DataPoint> prepareErrorAnalysisData() const;
    
    // 准备残差图数据
    std::vector<DataPoint> prepareResidualData() const;
    
    // ===== 数据插值（用于平滑曲线） =====
    
    // 线性插值
    std::vector<DataPoint> interpolateLinear(
        const std::vector<DataPoint>& points, int numPoints) const;
    
    // 样条插值
    std::vector<DataPoint> interpolateSpline(
        const std::vector<DataPoint>& points, int numPoints) const;
    
    // ===== 数据导出 =====
    
    // 导出分析结果为CSV
    bool exportAnalysisResults(const std::string& filename,
                               const DataStatistics& stats,
                               const ErrorStatistics& errors) const;
    
    // ===== 辅助方法 =====
    
    // 查找最接近的值
    std::vector<MeasurementData> findClosestValues(
        double targetHeight, double targetAngle, double targetTemp,
        int maxCount = 10) const;
    
    // 获取唯一值（用于确定固定值选项）
    std::vector<double> getUniqueHeights(double tolerance = 0.5) const;
    std::vector<double> getUniqueAngles(double tolerance = 0.5) const;
    std::vector<double> getUniqueTemperatures(double tolerance = 0.1) const;
    
    // 数据有效性检查
    bool hasValidData() const;
    
    // 获取数据范围
    struct DataRange {
        double minHeight, maxHeight;
        double minAngle, maxAngle;
        double minTemp, maxTemp;
        double minCapacitance, maxCapacitance;
    };
    DataRange getDataRange() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // 辅助计算方法
    double calculateMean(const std::vector<double>& values) const;
    double calculateStdDev(const std::vector<double>& values, double mean) const;
    double calculateRMSE(const std::vector<double>& actual, 
                        const std::vector<double>& predicted) const;
    double calculateR2(const std::vector<double>& actual,
                       const std::vector<double>& predicted) const;
    
    // 数据提取辅助方法
    std::vector<double> extractHeights(const std::vector<MeasurementData>& data) const;
    std::vector<double> extractAngles(const std::vector<MeasurementData>& data) const;
    std::vector<double> extractTemperatures(const std::vector<MeasurementData>& data) const;
    std::vector<double> extractMeasuredCapacitances(const std::vector<MeasurementData>& data) const;
    std::vector<double> extractTheoreticalCapacitances(const std::vector<MeasurementData>& data) const;
    
    // 排序辅助方法
    void sortDataByHeight(std::vector<MeasurementData>& data) const;
    void sortDataByAngle(std::vector<MeasurementData>& data) const;
    void sortDataByTemperature(std::vector<MeasurementData>& data) const;
};

#endif // CSV_ANALYZER_H