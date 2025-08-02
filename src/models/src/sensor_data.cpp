// src/models/src/sensor_data.cpp
#include "../include/sensor_data.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// 定义传感器范围常量
namespace {
    const double UPPER_SENSOR_MIN = 0.0;
    const double UPPER_SENSOR_MAX = 100.0;
    const double LOWER_SENSOR_MIN = 0.0;
    const double LOWER_SENSOR_MAX = 200.0;
    const double TEMPERATURE_MIN = -40.0;
    const double TEMPERATURE_MAX = 85.0;
    const double ANGLE_MIN = -90.0;
    const double ANGLE_MAX = 90.0;
    const double CAPACITANCE_MIN = 0.0;
    const double CAPACITANCE_MAX = 1000.0;
}

SensorData::SensorData() 
    : upperSensor1(0.0), upperSensor2(0.0),
      lowerSensor1(0.0), lowerSensor2(0.0),
      temperature(0.0), measuredAngle(0.0),
      measuredCapacitance(0.0), valid(false) {
}

SensorData::SensorData(double upper1, double upper2, double lower1, double lower2,
                       double temp, double angle, double capacitance)
    : upperSensor1(upper1), upperSensor2(upper2),
      lowerSensor1(lower1), lowerSensor2(lower2),
      temperature(temp), measuredAngle(angle),
      measuredCapacitance(capacitance) {
    // 检查数据有效性
    valid = isValid();
}

SensorData::SensorData(const SensorData& other) {
    *this = other;
}

SensorData& SensorData::operator=(const SensorData& other) {
    if (this != &other) {
        upperSensor1 = other.upperSensor1;
        upperSensor2 = other.upperSensor2;
        lowerSensor1 = other.lowerSensor1;
        lowerSensor2 = other.lowerSensor2;
        temperature = other.temperature;
        measuredAngle = other.measuredAngle;
        measuredCapacitance = other.measuredCapacitance;
        systemHeight = other.systemHeight;
        middlePlateHeight = other.middlePlateHeight;
        sensorSpacing = other.sensorSpacing;
        valid = other.valid;
    }
    return *this;
}

double SensorData::getAverageHeight() const {
    return (upperSensor1 + upperSensor2) / 2.0;
}

double SensorData::getCalculatedAngle() const {
    double heightDiff = upperSensor2 - upperSensor1;
    return std::atan(heightDiff / sensorSpacing) * 180.0 / M_PI;
}

double SensorData::getAverageGroundDistance() const {
    return (lowerSensor1 + lowerSensor2) / 2.0;
}

double SensorData::getCalculatedUpperDistance() const {
    return systemHeight - getAverageGroundDistance() - middlePlateHeight - getAverageHeight();
}

bool SensorData::setUpperSensors(double sensor1, double sensor2) {
    if (isInRange(sensor1, UPPER_SENSOR_MIN, UPPER_SENSOR_MAX) &&
        isInRange(sensor2, UPPER_SENSOR_MIN, UPPER_SENSOR_MAX)) {
        upperSensor1 = sensor1;
        upperSensor2 = sensor2;
        valid = isValid();
        return true;
    }
    return false;
}

bool SensorData::setLowerSensors(double sensor1, double sensor2) {
    if (isInRange(sensor1, LOWER_SENSOR_MIN, LOWER_SENSOR_MAX) &&
        isInRange(sensor2, LOWER_SENSOR_MIN, LOWER_SENSOR_MAX)) {
        lowerSensor1 = sensor1;
        lowerSensor2 = sensor2;
        valid = isValid();
        return true;
    }
    return false;
}

bool SensorData::setTemperature(double temp) {
    if (isInRange(temp, TEMPERATURE_MIN, TEMPERATURE_MAX)) {
        temperature = temp;
        valid = isValid();
        return true;
    }
    return false;
}

bool SensorData::setMeasuredAngle(double angle) {
    if (isInRange(angle, ANGLE_MIN, ANGLE_MAX)) {
        measuredAngle = angle;
        valid = isValid();
        return true;
    }
    return false;
}

bool SensorData::setMeasuredCapacitance(double cap) {
    if (isInRange(cap, CAPACITANCE_MIN, CAPACITANCE_MAX)) {
        measuredCapacitance = cap;
        valid = isValid();
        return true;
    }
    return false;
}

bool SensorData::isValid() const {
    return isInRange(upperSensor1, UPPER_SENSOR_MIN, UPPER_SENSOR_MAX) &&
           isInRange(upperSensor2, UPPER_SENSOR_MIN, UPPER_SENSOR_MAX) &&
           isInRange(lowerSensor1, LOWER_SENSOR_MIN, LOWER_SENSOR_MAX) &&
           isInRange(lowerSensor2, LOWER_SENSOR_MIN, LOWER_SENSOR_MAX) &&
           isInRange(temperature, TEMPERATURE_MIN, TEMPERATURE_MAX) &&
           isInRange(measuredAngle, ANGLE_MIN, ANGLE_MAX) &&
           isInRange(measuredCapacitance, CAPACITANCE_MIN, CAPACITANCE_MAX);
}

bool SensorData::parseFromString(const std::string& dataString) {
    if (dataString.empty()) {
        return false;
    }
    
    std::vector<double> values = parseNumbers(dataString);
    
    // 需要7个值
    if (values.size() != 7) {
        return false;
    }
    
    // 临时存储，以免部分失败导致数据不一致
    double temp_upper1 = values[0];
    double temp_upper2 = values[1];
    double temp_lower1 = values[2];
    double temp_lower2 = values[3];
    double temp_temp = values[4];
    double temp_angle = values[5];
    double temp_cap = values[6];
    
    // 检查范围
    if (!isInRange(temp_upper1, UPPER_SENSOR_MIN, UPPER_SENSOR_MAX) ||
        !isInRange(temp_upper2, UPPER_SENSOR_MIN, UPPER_SENSOR_MAX) ||
        !isInRange(temp_lower1, LOWER_SENSOR_MIN, LOWER_SENSOR_MAX) ||
        !isInRange(temp_lower2, LOWER_SENSOR_MIN, LOWER_SENSOR_MAX) ||
        !isInRange(temp_temp, TEMPERATURE_MIN, TEMPERATURE_MAX) ||
        !isInRange(temp_angle, ANGLE_MIN, ANGLE_MAX) ||
        !isInRange(temp_cap, CAPACITANCE_MIN, CAPACITANCE_MAX)) {
        return false;
    }
    
    // 全部检查通过后再赋值
    upperSensor1 = temp_upper1;
    upperSensor2 = temp_upper2;
    lowerSensor1 = temp_lower1;
    lowerSensor2 = temp_lower2;
    temperature = temp_temp;
    measuredAngle = temp_angle;
    measuredCapacitance = temp_cap;
    valid = true;
    
    return true;
}

std::string SensorData::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "SensorData{";
    oss << "upper:[" << upperSensor1 << "," << upperSensor2 << "]mm, ";
    oss << "lower:[" << lowerSensor1 << "," << lowerSensor2 << "]mm, ";
    oss << "temp:" << temperature << "°C, ";
    oss << "angle:" << measuredAngle << "°, ";
    oss << "cap:" << measuredCapacitance << "pF";
    oss << "}";
    return oss.str();
}

std::string SensorData::toCSV() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << upperSensor1 << ",";
    oss << upperSensor2 << ",";
    oss << lowerSensor1 << ",";
    oss << lowerSensor2 << ",";
    oss << temperature << ",";
    oss << measuredAngle << ",";
    oss << measuredCapacitance << ",";
    oss << getAverageHeight() << ",";
    oss << getCalculatedAngle() << ",";
    oss << getAverageGroundDistance() << ",";
    oss << getCalculatedUpperDistance();
    return oss.str();
}

std::string SensorData::getCSVHeader() {
    return "Upper_Sensor_1(mm),Upper_Sensor_2(mm),Lower_Sensor_1(mm),Lower_Sensor_2(mm),"
           "Temperature(C),Measured_Angle(deg),Measured_Capacitance(pF),"
           "Average_Height(mm),Calculated_Angle(deg),Average_Ground_Distance(mm),"
           "Calculated_Upper_Distance(mm)";
}

bool SensorData::isInRange(double value, double min, double max) const {
    return value >= min && value <= max;
}

std::vector<double> SensorData::parseNumbers(const std::string& str) const {
    std::vector<double> result;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        try {
            // 去除前后空格
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            
            double value = std::stod(token);
            result.push_back(value);
        } catch (const std::exception& e) {
            // 解析失败，返回空向量
            return std::vector<double>();
        }
    }
    
    return result;
}