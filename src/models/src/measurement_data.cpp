#include "../include/measurement_data.h"
#include "../../utils/include/time_utils.h"
#include "../include/physics_calculator.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <ctime>

MeasurementData::MeasurementData() 
    : timestamp(TimeUtils::getCurrentTimestamp()),
      m_setHeight(0.0),
      m_setAngle(0.0),
      theoreticalCapacitance(0.0) {
    theoreticalCapacitance = PhysicsCalculator::calculateParallelPlateCapacitance(plateArea, m_setHeight, m_setAngle, dielectricConstant);
}

MeasurementData::MeasurementData(double height, double angle, const SensorData& sensorData)
    : timestamp(TimeUtils::getCurrentTimestamp()),
      m_setHeight(height),
      m_setAngle(angle),
      sensorData(sensorData) {
    theoreticalCapacitance = PhysicsCalculator::calculateParallelPlateCapacitance(plateArea, m_setHeight, m_setAngle, dielectricConstant);
}

MeasurementData::MeasurementData(const MeasurementData& other) {
    *this = other;
}

MeasurementData& MeasurementData::operator=(const MeasurementData& other) {
    if (this != &other) {
        timestamp = other.timestamp;
        m_setHeight = other.m_setHeight;
        m_setAngle = other.m_setAngle;
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
    return sensorData.capacitance - theoreticalCapacitance;
}

bool MeasurementData::setHeight(double height) {
    if (isInSafetyRange(height, m_setAngle)) {
        m_setHeight = height;
        theoreticalCapacitance = PhysicsCalculator::calculateParallelPlateCapacitance(
    plateArea, height, m_setAngle, dielectricConstant);
        return true;
    }
    return false;
}

bool MeasurementData::setAngle(double angle) {
    if (isInSafetyRange(m_setHeight, angle)) {
        m_setAngle = angle;
        theoreticalCapacitance = PhysicsCalculator::calculateParallelPlateCapacitance(
    plateArea, m_setHeight, angle, dielectricConstant);
        return true;
    }
    return false;
}

void MeasurementData::setPlateArea(double area) {
    plateArea = area; 
    theoreticalCapacitance = PhysicsCalculator::calculateParallelPlateCapacitance(
        plateArea, m_setHeight, m_setAngle, dielectricConstant);
}

void MeasurementData::setDielectricConstant(double epsilon) { 
    dielectricConstant = epsilon; 
    theoreticalCapacitance = PhysicsCalculator::calculateParallelPlateCapacitance(
        plateArea, m_setHeight, m_setAngle, dielectricConstant);
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
    return sensorData.hasValidData() && 
           isInSafetyRange(m_setHeight, m_setAngle) &&
           theoreticalCapacitance > 0;
}

std::string MeasurementData::getFormattedTime() const {
    return TimeUtils::formatTimestamp(timestamp);
}

std::string MeasurementData::toCSV() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    // 时间戳
    oss << TimeUtils::formatTimestamp(timestamp) << ",";

    // 设定值
    oss << m_setHeight << ",";
    oss << m_setAngle << ",";
    oss << theoreticalCapacitance << ",";
    
    // 传感器原始数据
    oss << sensorData.distanceUpper1 << ",";
    oss << sensorData.distanceUpper2 << ",";
    oss << sensorData.distanceLower1 << ",";
    oss << sensorData.distanceLower2 << ",";
    oss << sensorData.temperature << ",";
    oss << sensorData.angle << ",";
    oss << sensorData.capacitance << ",";
    
    // 计算值
    oss << sensorData.getAverageHeight() << ",";
    oss << sensorData.getCalculatedAngle() << ",";
    oss << sensorData.getAverageGroundDistance() << ",";
    oss << sensorData.getCalculatedUpperDistance() << ",";
    oss << getCapacitanceDifference();
    
    return oss.str();
}

std::string MeasurementData::getCSVHeader() {
    return "Timestamp,"
           "Set_Height(mm),Set_Angle(deg),Theoretical_Capacitance(pF),"
           "Upper_Sensor_1(mm),Upper_Sensor_2(mm),"
           "Lower_Sensor_1(mm),Lower_Sensor_2(mm),"
           "Temperature(C),Measured_Angle(deg),Measured_Capacitance(pF),"
           "Average_Height(mm),Calculated_Angle(deg),"
           "Average_Ground_Distance(mm),Calculated_Upper_Distance(mm),"
           "Capacitance_Difference(pF)";
}

std::string MeasurementData::toLogString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Timestamp: " << TimeUtils::formatTimestamp(timestamp) << "\n";
    oss << "Set Values: Height=" << m_setHeight << "mm, Angle=" << m_setAngle << "°\n";
    oss << "Theoretical Capacitance: " << theoreticalCapacitance << "pF\n";
    oss << "Sensor Data: " << sensorData.toString() << "\n";
    oss << "Capacitance Difference: " << getCapacitanceDifference() << "pF";
    return oss.str();
}

bool MeasurementData::isInSafetyRange(double height, double angle) const {
    return height >= minHeight && height <= maxHeight &&
           angle >= minAngle && angle <= maxAngle;
}