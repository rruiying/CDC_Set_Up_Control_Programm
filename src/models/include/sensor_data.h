// src/models/include/sensor_data.h
#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <string>
#include <vector>

/**
 * @brief 传感器数据类
 * 
 * 用于存储和管理所有传感器的测量数据，包括：
 * - 4个距离传感器（上方2个，下方2个）
 * - 温度传感器
 * - 角度传感器（SCA830）
 * - 电容传感器
 */
class SensorData {
public:
    /**
     * @brief 默认构造函数
     */
    SensorData();
    
    /**
     * @brief 带参数的构造函数
     * @param upper1 上方传感器1的读数(mm)
     * @param upper2 上方传感器2的读数(mm)
     * @param lower1 下方传感器1的读数(mm)
     * @param lower2 下方传感器2的读数(mm)
     * @param temp 温度读数(°C)
     * @param angle 角度传感器读数(度)
     * @param capacitance 电容读数(pF)
     */
    SensorData(double upper1, double upper2, double lower1, double lower2,
               double temp, double angle, double capacitance);
    
    /**
     * @brief 拷贝构造函数
     */
    SensorData(const SensorData& other);
    
    /**
     * @brief 赋值运算符
     */
    SensorData& operator=(const SensorData& other);
    
    // Getter方法
    double getUpperSensor1() const { return upperSensor1; }
    double getUpperSensor2() const { return upperSensor2; }
    double getLowerSensor1() const { return lowerSensor1; }
    double getLowerSensor2() const { return lowerSensor2; }
    double getTemperature() const { return temperature; }
    double getMeasuredAngle() const { return measuredAngle; }
    double getMeasuredCapacitance() const { return measuredCapacitance; }
    
    // 计算值的Getter方法
    double getAverageHeight() const;
    double getCalculatedAngle() const;
    double getAverageGroundDistance() const;
    double getCalculatedUpperDistance() const;
    
    // Setter方法（带范围检查）
    bool setUpperSensors(double sensor1, double sensor2);
    bool setLowerSensors(double sensor1, double sensor2);
    bool setTemperature(double temp);
    bool setMeasuredAngle(double angle);
    bool setMeasuredCapacitance(double cap);
    
    // 系统参数设置
    void setSystemHeight(double height) { systemHeight = height; }
    void setMiddlePlateHeight(double height) { middlePlateHeight = height; }
    void setSensorSpacing(double spacing) { sensorSpacing = spacing; }
    
    /**
     * @brief 检查数据是否有效
     * @return true 如果所有数据都在合理范围内
     */
    bool isValid() const;
    
    /**
     * @brief 从字符串解析数据（用于串口通信）
     * @param dataString 格式："upper1,upper2,lower1,lower2,temp,angle,cap"
     * @return true 如果解析成功
     */
    bool parseFromString(const std::string& dataString);
    
    /**
     * @brief 转换为字符串（用于日志记录）
     * @return 格式化的字符串
     */
    std::string toString() const;
    
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
    
private:
    // 原始传感器数据
    double upperSensor1;    // 上方传感器1 (mm)
    double upperSensor2;    // 上方传感器2 (mm)
    double lowerSensor1;    // 下方传感器1 (mm)
    double lowerSensor2;    // 下方传感器2 (mm)
    double temperature;     // 温度 (°C)
    double measuredAngle;   // 测量角度 (度)
    double measuredCapacitance; // 测量电容 (pF)
    
    // 系统参数
    double systemHeight = 200.0;      // 系统总高度 (mm)
    double middlePlateHeight = 25.0;  // 中间板高度 (mm)
    double sensorSpacing = 100.0;     // 传感器间距 (mm)
    
    // 数据有效性标志
    bool valid = false;
    
    // 辅助方法
    bool isInRange(double value, double min, double max) const;
    std::vector<double> parseNumbers(const std::string& str) const;
};

#endif // SENSOR_DATA_H