// src/models/include/data_statistics.h
#ifndef DATA_STATISTICS_H
#define DATA_STATISTICS_H

#include <cstddef>
#include <cstdint>

/**
 * @brief 统一的数据统计结构体
 * 用于数据记录器、数据处理器和CSV分析器
 */
struct DataStatistics {
    // 数据计数
    int dataCount = 0;
    
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
    
    // 电容统计
    double meanCapacitance = 0.0;
    double stdDevCapacitance = 0.0;
    double minCapacitance = 0.0;
    double maxCapacitance = 0.0;
    
    // 温度统计
    double meanTemperature = 0.0;
    double stdDevTemperature = 0.0;
    double minTemperature = 0.0;
    double maxTemperature = 0.0;
    
    // 时间相关（用于DataRecorder）
    int64_t firstRecordTime = 0;
    int64_t lastRecordTime = 0;
    
    // 额外的统计信息（用于DataProcessor）
    double variance = 0.0;
    double skewness = 0.0;
    double kurtosis = 0.0;
    double median = 0.0;
    
    // 兼容旧版本的别名
    int totalRecords() const { return static_cast<int>(dataCount); }
    double averageHeight() const { return meanHeight; }
    double averageAngle() const { return meanAngle; }
    double averageCapacitance() const { return meanCapacitance; }
};

#endif // DATA_STATISTICS_H