#ifndef MEASUREMENT_DATA_H
#define MEASUREMENT_DATA_H

#include "sensor_data.h"
#include "physics_calculator.h"
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <ctime>

class MeasurementData {
public:

    MeasurementData();
    
    MeasurementData(double height, double angle, const SensorData& sensorData);
    
    MeasurementData(const MeasurementData& other);
    
    MeasurementData& operator=(const MeasurementData& other);
    
    // Getter方法
    int64_t getTimestamp() const { return timestamp; }
    double getSetHeight() const { return m_setHeight; }
    double getSetAngle() const { return m_setAngle; }
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
    void setPlateArea(double area);
    void setDielectricConstant(double epsilon);

    // 安全限位设置
    void setSafetyLimits(double minHeight, double maxHeight, double minAngle, double maxAngle);
    
    bool isValid() const;
    
    std::string getFormattedTime() const;
    
    std::string toCSV() const;
    
    static std::string getCSVHeader();
    
    std::string toLogString() const;
    
private:
    // 时间戳（毫秒）
    int64_t timestamp;
    
    // 设定值
    double m_setHeight;           // 设定高度 (mm)
    double m_setAngle;            // 设定角度 (度)
    
    // 传感器数据
    SensorData sensorData;
    
    // 理论计算值
    double theoreticalCapacitance;  // 理论电容值 (pF)
    
    // 电容板参数
    double plateArea = 2500.0;   // mm² (50mm x 50mm)
    double dielectricConstant = 1.0; // 相对介电常数
    
    // 安全限位
    double minHeight = 0.0;
    double maxHeight = 150.0;
    double minAngle = -90.0;
    double maxAngle = 90.0;
    
    bool isInSafetyRange(double height, double angle) const;
};

#endif // MEASUREMENT_DATA_H