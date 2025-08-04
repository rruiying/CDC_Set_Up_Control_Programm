// src/models/include/sensor_data.h
#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <QString>
#include <QDateTime>
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
     * @brief 拷贝构造函数
     */
    SensorData(const SensorData& other);
    
    /**
     * @brief 赋值运算符
     */
    SensorData& operator=(const SensorData& other);
    
    // 原始传感器数据
    double distanceUpper1;    // 上方传感器1 (mm)
    double distanceUpper2;    // 上方传感器2 (mm)
    double distanceLower1;    // 下方传感器1 (mm)
    double distanceLower2;    // 下方传感器2 (mm)
    double temperature;       // 温度 (°C)
    double angle;            // 测量角度 (度)
    double capacitance;      // 测量电容 (pF)
    
    // 时间戳
    QDateTime timestamp;
    
    // 数据有效性标志
    struct {
        bool distanceUpper1;
        bool distanceUpper2;
        bool distanceLower1;
        bool distanceLower2;
        bool temperature;
        bool angle;
        bool capacitance;
    } isValid;
    
    // 系统参数
    double systemHeight = 200.0;      // 系统总高度 (mm)
    double middlePlateHeight = 25.0;  // 中间板高度 (mm)
    double sensorSpacing = 100.0;     // 传感器间距 (mm)
    
    // 计算值的方法
    double getAverageHeight() const;
    double getCalculatedAngle() const;
    double getAverageGroundDistance() const;
    double getCalculatedUpperDistance() const;
    
    // Setter方法（带范围检查）
    bool setUpperSensors(double sensor1, double sensor2);
    bool setLowerSensors(double sensor1, double sensor2);
    bool setTemperature(double temp);
    bool setAngle(double angle);
    bool setCapacitance(double cap);
    
    // 系统参数设置
    void setSystemHeight(double height) { systemHeight = height; }
    void setMiddlePlateHeight(double height) { middlePlateHeight = height; }
    void setSensorSpacing(double spacing) { sensorSpacing = spacing; }
    
    /**
     * @brief 检查所有数据是否有效
     * @return true 如果所有数据都在合理范围内
     */
    bool isAllValid() const;
    
    /**
     * @brief 检查是否有任何有效数据
     * @return true 如果至少有一个数据有效
     */
    bool hasValidData() const;
    
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
    QString toQString() const;
    
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
     * @brief 重置所有数据
     */
    void reset();
    
private:
    // 辅助方法
    bool isInRange(double value, double min, double max) const;
    std::vector<double> parseNumbers(const std::string& str) const;
};

#endif // SENSOR_DATA_H