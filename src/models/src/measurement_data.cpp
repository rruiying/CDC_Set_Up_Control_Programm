// src/models/src/measurement_data.cpp
#include "../include/measurement_data.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <ctime>

MeasurementData::MeasurementData() 
    : timestamp(getCurrentTimestamp()),
      setHeight(0.0),
      setAngle(0.0),
      theoreticalCapacitance(0.0) {
    calculateTheoreticalCapacitance();
}

MeasurementData::MeasurementData(double setHeight, double setAngle, const SensorData& sensorData)
    : timestamp(getCurrentTimestamp()),
      setHeight(setHeight),
      setAngle(setAngle),
      sensorData(sensorData) {
    calculateTheoreticalCapacitance();
}

MeasurementData::MeasurementData(const MeasurementData& other) {
    *this = other;
}

MeasurementData& MeasurementData::operator=(const MeasurementData& other) {
    if (this != &other) {
        timestamp = other.timestamp;
        setHeight = other.setHeight;
        setAngle = other.setAngle;
        sensorData = other.sensorData;
        theoreticalCapacitance = other.theoreticalCapacitance;
        plateArea = other.plateArea;
        dielectricConstant = other.dielectricConstant;
        minHeight = other.minHeight;
        maxHeight = other.maxHeight;
        minAngle = other.minAngle;
        maxAngle = other.maxAngle;
    }
    return *this;
}

double MeasurementData::getCapacitanceDifference() const {
    return sensorData.getMeasuredCapacitance() - theoreticalCapacitance;
}

bool MeasurementData::setHeight(double height) {
    if (isInSafetyRange(height, setAngle)) {
        setHeight = height;
        calculateTheoreticalCapacitance();
        return true;
    }
    return false;
}

bool MeasurementData::setAngle(double angle) {
    if (isInSafetyRange(setHeight, angle)) {
        setAngle = angle;
        calculateTheoreticalCapacitance();
        return true;
    }
    return false;
}

void MeasurementData::updateSensorData(const SensorData& data) {
    sensorData = data;
}

void MeasurementData::setSafetyLimits(double minH, double maxH, double minA, double maxA) {
    minHeight = minH;
    maxHeight = maxH;
    minAngle = minA;
    maxAngle = maxA;
}

bool MeasurementData::isValid() const {
    return sensorData.isValid() && 
           isInSafetyRange(setHeight, setAngle) &&
           theoreticalCapacitance > 0;
}

std::string MeasurementData::getFormattedTime() const {
    // 将毫秒时间戳转换为秒
    std::time_t seconds = timestamp / 1000;
    int milliseconds = timestamp % 1000;
    
    // 转换为本地时间
    std::tm* tm = std::localtime(&seconds);
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << milliseconds;
    
    return oss.str();
}

std::string MeasurementData::toCSV() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    // 时间戳
    oss << getFormattedTime() << ",";
    
    // 设定值
    oss << setHeight << ",";
    oss << setAngle << ",";
    
    // 理论电容和差值
    oss << theoreticalCapacitance << ",";
    oss << getCapacitanceDifference() << ",";
    
    // 传感器数据（使用SensorData的CSV方法）
    oss << sensorData.toCSV();
    
    return oss.str();
}

std::string MeasurementData::getCSVHeader() {
    return "Timestamp,Set_Height(mm),Set_Angle(deg),"
           "Theoretical_Capacitance(pF),Capacitance_Difference(pF)," +
           SensorData::getCSVHeader();
}

std::string MeasurementData::toLogString() const {
    std::ostringstream oss;
    oss << "[" << getFormattedTime() << "] ";
    oss << "Height:" << std::fixed << std::setprecision(1) << setHeight << "mm ";
    oss << "Angle:" << setAngle << "° ";
    oss << "TheoCap:" << std::setprecision(2) << theoreticalCapacitance << "pF ";
    oss << "MeasCap:" << sensorData.getMeasuredCapacitance() << "pF ";
    oss << "Diff:" << std::showpos << getCapacitanceDifference() << "pF";
    return oss.str();
}

void MeasurementData::calculateTheoreticalCapacitance() {
    if (setHeight <= 0) {
        theoreticalCapacitance = 0.0;
        return;
    }
    
    // 将高度从mm转换为m
    double distanceInMeters = setHeight / 1000.0;
    
    // 计算有效面积（考虑倾斜角度）
    double angleInRadians = setAngle * M_PI / 180.0;
    double effectiveArea = plateArea * std::cos(angleInRadians);
    
    // C = ε₀ * εᵣ * A / d
    double capacitanceInFarads = EPSILON_0 * dielectricConstant * effectiveArea / distanceInMeters;
    
    // 转换为pF
    theoreticalCapacitance = capacitanceInFarads * 1e12;
}

int64_t MeasurementData::getCurrentTimestamp() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<milliseconds>(duration).count();
}

bool MeasurementData::isInSafetyRange(double height, double angle) const {
    return height >= minHeight && height <= maxHeight &&
           angle >= minAngle && angle <= maxAngle;
}