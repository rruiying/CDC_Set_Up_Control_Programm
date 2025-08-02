// src/models/include/measurement_data.h
#ifndef MEASUREMENT_DATA_H
#define MEASUREMENT_DATA_H

#include "sensor_data.h"
#include <string>
#include <cstdint>

/**
 * @brief 测量数据记录类
 * 
 * 包含一次完整的测量记录：
 * - 时间戳
 * - 设定值（高度、角度）
 * - 传感器测量值
 * - 理论计算值
 * - 误差分析
 */
class MeasurementData {
public:
    /**
     * @brief 默认构造函数
     * 使用当前时间作为时间戳
     */
    MeasurementData();
    
    /**
     * @brief 带参数的构造函数
     * @param setHeight 设定高度(mm)
     * @param setAngle 设定角度(度)
     * @param sensorData 传感器数据
     */
    MeasurementData(double setHeight, double setAngle, const SensorData& sensorData);
    
    /**
     * @brief 拷贝构造函数
     */
    MeasurementData(const MeasurementData& other);
    
    /**
     * @brief 赋值运算符
     */
    MeasurementData& operator=(const MeasurementData& other);
    
    // Getter方法
    int64_t getTimestamp() const { return timestamp; }
    double getSetHeight() const { return setHeight; }
    double getSetAngle() const { return setAngle; }
    double getTheoreticalCapacitance() const { return theoreticalCapacitance; }
    double getCapacitanceDifference() const;
    const SensorData& getSensorData() const { return sensorData; }
    double getPlateArea() const { return plateArea; }
    double getDielectricConstant() const { return dielectricConstant; }
    
    // Setter方法
    void setTimestamp(int64_t ts) { timestamp = ts; }
    bool setHeight(double height);
    bool setAngle(double angle);
    void updateSensorData(const SensorData& data);
    
    // 电容板参数设置
    void setPlateArea(double area) { plateArea = area; calculateTheoreticalCapacitance(); }
    void setDielectricConstant(double epsilon) { dielectricConstant = epsilon; calculateTheoreticalCapacitance(); }
    
    // 安全限位设置
    void setSafetyLimits(double minHeight, double maxHeight, double minAngle, double maxAngle);
    
    /**
     * @brief 检查数据是否有效
     * @return true 如果所有数据都有效且在安全范围内
     */
    bool isValid() const;
    
    /**
     * @brief 获取格式化的时间字符串
     * @return 格式："YYYY-MM-DD HH:MM:SS.mmm"
     */
    std::string getFormattedTime() const;
    
    /**
     * @brief 转换为CSV格式
     * @return CSV格式的字符串
     */
    std::string toCSV() const;
    
    /**
     * @brief 获取CSV头部
     * @return CSV列名
     */
    static std::string getCSVHeader();
    
    /**
     * @brief 转换为日志格式
     * @return 适合日志记录的格式化字符串
     */
    std::string toLogString() const;
    
private:
    // 时间戳（毫秒）
    int64_t timestamp;
    
    // 设定值
    double setHeight;           // 设定高度 (mm)
    double setAngle;            // 设定角度 (度)
    
    // 传感器数据
    SensorData sensorData;
    
    // 理论计算值
    double theoreticalCapacitance;  // 理论电容值 (pF)
    
    // 电容板参数
    double plateArea = 0.01;        // 电容板面积 (m²)
    double dielectricConstant = 1.0; // 相对介电常数
    
    // 安全限位
    double minHeight = 0.0;
    double maxHeight = 100.0;
    double minAngle = -90.0;
    double maxAngle = 90.0;
    
    // 物理常数
    static constexpr double EPSILON_0 = 8.854e-12; // 真空介电常数 (F/m)
    
    // 辅助方法
    void calculateTheoreticalCapacitance();
    int64_t getCurrentTimestamp() const;
    bool isInSafetyRange(double height, double angle) const;
};

#endif // MEASUREMENT_DATA_H