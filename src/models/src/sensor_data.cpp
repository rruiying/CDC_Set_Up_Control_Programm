// src/models/src/sensor_data.cpp
#include "../include/sensor_data.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// 定义传感器范围常量
namespace {
    const double DISTANCE_SENSOR_MIN = 0.0;
    const double DISTANCE_SENSOR_MAX = 300.0;
    const double TEMPERATURE_MIN = -40.0;
    const double TEMPERATURE_MAX = 85.0;
    const double ANGLE_MIN = -90.0;
    const double ANGLE_MAX = 90.0;
    const double CAPACITANCE_MIN = 0.0;
    const double CAPACITANCE_MAX = 1000.0;
}

SensorData::SensorData() 
    : distanceUpper1(0.0), distanceUpper2(0.0),
      distanceLower1(0.0), distanceLower2(0.0),
      temperature(0.0), angle(0.0),
      capacitance(0.0) {
    reset();
}

SensorData::SensorData(const SensorData& other) {
    *this = other;
}

SensorData& SensorData::operator=(const SensorData& other) {
    if (this != &other) {
        distanceUpper1 = other.distanceUpper1;
        distanceUpper2 = other.distanceUpper2;
        distanceLower1 = other.distanceLower1;
        distanceLower2 = other.distanceLower2;
        temperature = other.temperature;
        angle = other.angle;
        capacitance = other.capacitance;
        timestamp = other.timestamp;
        isValid = other.isValid;
        systemHeight = other.systemHeight;
        middlePlateHeight = other.middlePlateHeight;
        sensorSpacing = other.sensorSpacing;
    }
    return *this;
}

void SensorData::reset() {
    distanceUpper1 = 0.0;
    distanceUpper2 = 0.0;
    distanceLower1 = 0.0;
    distanceLower2 = 0.0;
    temperature = 0.0;
    angle = 0.0;
    capacitance = 0.0;
    
    isValid.distanceUpper1 = false;
    isValid.distanceUpper2 = false;
    isValid.distanceLower1 = false;
    isValid.distanceLower2 = false;
    isValid.temperature = false;
    isValid.angle = false;
    isValid.capacitance = false;
}

double SensorData::getAverageHeight() const {
    if (isValid.distanceUpper1 && isValid.distanceUpper2) {
        return (distanceUpper1 + distanceUpper2) / 2.0;
    } else if (isValid.distanceUpper1) {
        return distanceUpper1;
    } else if (isValid.distanceUpper2) {
        return distanceUpper2;
    }
    return 0.0;
}

double SensorData::getCalculatedAngle() const {
    if (isValid.distanceUpper1 && isValid.distanceUpper2 && sensorSpacing > 0) {
        double heightDiff = distanceUpper2 - distanceUpper1;
        return std::atan(heightDiff / sensorSpacing) * 180.0 / M_PI;
    }
    return 0.0;
}

double SensorData::getAverageGroundDistance() const {
    if (isValid.distanceLower1 && isValid.distanceLower2) {
        return (distanceLower1 + distanceLower2) / 2.0;
    } else if (isValid.distanceLower1) {
        return distanceLower1;
    } else if (isValid.distanceLower2) {
        return distanceLower2;
    }
    return 0.0;
}

double SensorData::getCalculatedUpperDistance() const {
    return systemHeight - getAverageGroundDistance() - middlePlateHeight - getAverageHeight();
}

bool SensorData::setUpperSensors(double sensor1, double sensor2) {
    if (isInRange(sensor1, DISTANCE_SENSOR_MIN, DISTANCE_SENSOR_MAX) &&
        isInRange(sensor2, DISTANCE_SENSOR_MIN, DISTANCE_SENSOR_MAX)) {
        distanceUpper1 = sensor1;
        distanceUpper2 = sensor2;
        isValid.distanceUpper1 = true;
        isValid.distanceUpper2 = true;
        return true;
    }
    return false;
}

bool SensorData::setLowerSensors(double sensor1, double sensor2) {
    if (isInRange(sensor1, DISTANCE_SENSOR_MIN, DISTANCE_SENSOR_MAX) &&
        isInRange(sensor2, DISTANCE_SENSOR_MIN, DISTANCE_SENSOR_MAX)) {
        distanceLower1 = sensor1;
        distanceLower2 = sensor2;
        isValid.distanceLower1 = true;
        isValid.distanceLower2 = true;
        return true;
    }
    return false;
}

bool SensorData::setTemperature(double temp) {
    if (isInRange(temp, TEMPERATURE_MIN, TEMPERATURE_MAX)) {
        temperature = temp;
        isValid.temperature = true;
        return true;
    }
    return false;
}

bool SensorData::setAngle(double a) {
    if (isInRange(a, ANGLE_MIN, ANGLE_MAX)) {
        angle = a;
        isValid.angle = true;
        return true;
    }
    return false;
}

bool SensorData::setCapacitance(double cap) {
    if (isInRange(cap, CAPACITANCE_MIN, CAPACITANCE_MAX)) {
        capacitance = cap;
        isValid.capacitance = true;
        return true;
    }
    return false;
}

bool SensorData::isAllValid() const {
    return isValid.distanceUpper1 && isValid.distanceUpper2 &&
           isValid.distanceLower1 && isValid.distanceLower2 &&
           isValid.temperature && isValid.angle && isValid.capacitance;
}

bool SensorData::hasValidData() const {
    return isValid.distanceUpper1 || isValid.distanceUpper2 ||
           isValid.distanceLower1 || isValid.distanceLower2 ||
           isValid.temperature || isValid.angle || isValid.capacitance;
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
    
    // 使用setter方法设置数据（包含范围检查）
    bool success = true;
    success &= setUpperSensors(values[0], values[1]);
    success &= setLowerSensors(values[2], values[3]);
    success &= setTemperature(values[4]);
    success &= setAngle(values[5]);
    success &= setCapacitance(values[6]);
    
    return success;
}

std::string SensorData::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "SensorData{";
    
    if (isValid.distanceUpper1 || isValid.distanceUpper2) {
        oss << "upper:[" << distanceUpper1 << "," << distanceUpper2 << "]mm, ";
    }
    if (isValid.distanceLower1 || isValid.distanceLower2) {
        oss << "lower:[" << distanceLower1 << "," << distanceLower2 << "]mm, ";
    }
    if (isValid.temperature) {
        oss << "temp:" << temperature << "°C, ";
    }
    if (isValid.angle) {
        oss << "angle:" << angle << "°, ";
    }
    if (isValid.capacitance) {
        oss << "cap:" << capacitance << "pF";
    }
    oss << "}";
    return oss.str();
}

QString SensorData::toQString() const {
    return QString::fromStdString(toString());
}

std::string SensorData::toCSV() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    // 时间戳
    if (timestamp.isValid()) {
        oss << timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz").toStdString() << ",";
    } else {
        oss << ",";
    }
    
    // 原始数据
    oss << distanceUpper1 << ",";
    oss << distanceUpper2 << ",";
    oss << distanceLower1 << ",";
    oss << distanceLower2 << ",";
    oss << temperature << ",";
    oss << angle << ",";
    oss << capacitance << ",";
    
    // 计算值
    oss << getAverageHeight() << ",";
    oss << getCalculatedAngle() << ",";
    oss << getAverageGroundDistance() << ",";
    oss << getCalculatedUpperDistance();
    
    return oss.str();
}

std::string SensorData::getCSVHeader() {
    return "Timestamp,Upper_Sensor_1(mm),Upper_Sensor_2(mm),Lower_Sensor_1(mm),Lower_Sensor_2(mm),"
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