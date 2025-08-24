#include "../include/sensor_data.h"
#include "../include/system_config.h"
#include "../../utils/include/time_utils.h"
#include "../include/physics_constants.h"
#include "../../utils/include/math_utils.h"
#include "../../utils/include/logger.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <ctime>

SensorData::SensorData() 
    : distanceUpper1(0.0), distanceUpper2(0.0),
      distanceLower1(0.0), distanceLower2(0.0),
      temperature(0.0), angle(0.0),
      capacitance(0.0),
      timestamp(TimeUtils::getCurrentTimestamp()) {
    const auto& config = SystemConfig::getInstance();

    
    // 初始化有效性标志
    isValid.distanceUpper1 = false;
    isValid.distanceUpper2 = false;
    isValid.distanceLower1 = false;
    isValid.distanceLower2 = false;
    isValid.temperature = false;
    isValid.angle = false;
    isValid.capacitance = false;
}

SensorData::SensorData(const SensorData& other)
:   distanceUpper1(other.distanceUpper1),
    distanceUpper2(other.distanceUpper2),
    distanceLower1(other.distanceLower1),
    distanceLower2(other.distanceLower2),
    temperature(other.temperature),
    angle(other.angle),
    capacitance(other.capacitance),
    timestamp(other.timestamp),
    isValid(other.isValid){
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
    timestamp = TimeUtils::getCurrentTimestamp();
    
    isValid.distanceUpper1 = false;
    isValid.distanceUpper2 = false;
    isValid.distanceLower1 = false;
    isValid.distanceLower2 = false;
    isValid.temperature = false;
    isValid.angle = false;
    isValid.capacitance = false;
}

double SensorData::getAverageHeight() const {
    bool d1_special = std::isnan(distanceUpper1) || std::isinf(distanceUpper1);
    bool d2_special = std::isnan(distanceUpper2) || std::isinf(distanceUpper2);
    
    if (isValid.distanceUpper1 && isValid.distanceUpper2 && !d1_special && !d2_special) {
        return (distanceUpper1 + distanceUpper2) / 2.0;
    } 
    else if (isValid.distanceUpper1 && !d1_special) {
        return distanceUpper1;
    } 
    else if (isValid.distanceUpper2 && !d2_special) {
        return distanceUpper2;
    }
    else if ((isValid.distanceUpper1 && std::isnan(distanceUpper1)) || 
             (isValid.distanceUpper2 && std::isnan(distanceUpper2))) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    else if ((isValid.distanceUpper1 && std::isinf(distanceUpper1)) || 
             (isValid.distanceUpper2 && std::isinf(distanceUpper2))) {
        if (std::isinf(distanceUpper1)) return distanceUpper1;
        if (std::isinf(distanceUpper2)) return distanceUpper2;
    }
    
    return 0.0;
}

double SensorData::getCalculatedAngle() const {
    if (isValid.distanceUpper1 && isValid.distanceUpper2) {
        double spacing = SystemConfig::getInstance().getSensorSpacing();
        if (spacing > 0) {
            double heightDiff = distanceUpper2 - distanceUpper1;
            double rad = std::atan(heightDiff / spacing);
            return rad * PhysicsConstants::RAD_TO_DEG;
        }
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
    const auto& config = SystemConfig::getInstance();
    return config.getTotalHeight() - getAverageGroundDistance() - 
           config.getMiddlePlateHeight() - getAverageHeight();
}

bool SensorData::setUpperSensors(double sensor1, double sensor2) {
        distanceUpper1 = sensor1;
        distanceUpper2 = sensor2;
        isValid.distanceUpper1 = true;
        isValid.distanceUpper2 = true;
        return true;
}

bool SensorData::setLowerSensors(double sensor1, double sensor2) {
    if (MathUtils::isInRange(sensor1, PhysicsConstants::DISTANCE_SENSOR_MIN, PhysicsConstants::DISTANCE_SENSOR_MAX) &&
        MathUtils::isInRange(sensor2, PhysicsConstants::DISTANCE_SENSOR_MIN, PhysicsConstants::DISTANCE_SENSOR_MAX)) {
        distanceLower1 = sensor1;
        distanceLower2 = sensor2;
        isValid.distanceLower1 = true;
        isValid.distanceLower2 = true;
        return true;
    }
    return false;
}

bool SensorData::setTemperature(double temp) {
        temperature = temp;
        isValid.temperature = true;
        return true;
}

bool SensorData::setAngle(double a) {
        angle = a;
        isValid.angle = true;
        return true;
}

bool SensorData::setCapacitance(double cap) {
        isValid.capacitance = true;
        return true;
}

bool SensorData::isAllValid() const {
    return true;
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

    if (values.size() != 7) {
        LOG_ERROR("Expected 7 values, got " + std::to_string(values.size()));
        return false;
    }
    distanceUpper1 = values[0];
    distanceUpper2 = values[1];
    distanceLower1 = values[2];
    distanceLower2 = values[3];
    temperature = values[4];
    angle = values[5];
    capacitance = values[6];
    isValid.distanceUpper1 = true;
    isValid.distanceUpper2 = true;
    isValid.distanceLower1 = true;
    isValid.distanceLower2 = true;
    isValid.temperature = true;
    isValid.angle = true;
    isValid.capacitance = true;

    if (std::isnan(distanceUpper1)) LOG_WARNING("Distance1 is NaN");
    if (std::isinf(distanceUpper1)) LOG_WARNING("Distance1 is Inf");
    if (std::isnan(temperature)) LOG_WARNING("Temperature is NaN");
    if (std::isinf(temperature)) LOG_WARNING("Temperature is Inf");
    if (std::isnan(angle)) LOG_WARNING("Angle is NaN");
    if (std::isinf(angle)) LOG_WARNING("Angle is Inf");
    
    return true;
}

std::string SensorData::toString() const {
    std::ostringstream oss;
    oss << "SensorData{";

    auto formatValue = [](double value, const std::string& unit) -> std::string {
        if (std::isnan(value)) return "NaN" + unit;
        if (std::isinf(value)) return (value > 0 ? "+Inf" : "-Inf") + unit;
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << value << unit;
        return ss.str();
    };
    
    if (isValid.distanceUpper1 || isValid.distanceUpper2) {
        oss << "upper:[" << formatValue(distanceUpper1, "") 
            << "," << formatValue(distanceUpper2, "") << "]mm, ";
    }
    
    if (isValid.temperature) {
        oss << "temp:" << formatValue(temperature, "°C") << ", ";
    }
    
    if (isValid.angle) {
        oss << "angle:" << formatValue(angle, "°") << ", ";
    }
    
    if (isValid.capacitance) {
        oss << "cap:" << formatValue(capacitance, "pF");
    }
    
    oss << "}";
    return oss.str();
}

std::string SensorData::toCSV() const {
    std::ostringstream oss;
    auto formatFloat = [](double value, int precision = 2) -> std::string {
        if (std::isnan(value)) {
            return "NaN";
        } else if (std::isinf(value)) {
            return value > 0 ? "Inf" : "-Inf";
        } else {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(precision) << value;
            return ss.str();
        }
    };
    
    oss << TimeUtils::formatTimestamp(timestamp) << ",";
    
    oss << formatFloat(distanceUpper1) << ",";
    oss << formatFloat(distanceUpper2) << ",";
    oss << formatFloat(distanceLower1) << ",";
    oss << formatFloat(distanceLower2) << ",";
    oss << formatFloat(temperature) << ",";
    oss << formatFloat(angle) << ",";
    oss << formatFloat(capacitance) << ",";
    
    oss << formatFloat(getAverageHeight()) << ",";
    oss << formatFloat(getCalculatedAngle()) << ",";
    oss << formatFloat(getAverageGroundDistance()) << ",";
    oss << formatFloat(getCalculatedUpperDistance());
    
    return oss.str();
}

std::string SensorData::getCSVHeader() {
    return "Timestamp,Upper_Sensor_1(mm),Upper_Sensor_2(mm),Lower_Sensor_1(mm),Lower_Sensor_2(mm),"
           "Temperature(C),Measured_Angle(deg),Measured_Capacitance(pF),"
           "Average_Height(mm),Calculated_Angle(deg),Average_Ground_Distance(mm),"
           "Calculated_Upper_Distance(mm)";
}

std::vector<double> SensorData::parseNumbers(const std::string& str) const {
    std::vector<double> result;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        try {
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            
            double value;
            
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            
            if (token == "nan" || token == "nan.0") {
                value = std::numeric_limits<double>::quiet_NaN();
            } else if (token == "inf" || token == "+inf" || token == "inf.0") {
                value = std::numeric_limits<double>::infinity();
            } else if (token == "-inf" || token == "-inf.0") {
                value = -std::numeric_limits<double>::infinity();
            } else {
                value = std::stod(token);
            }
            
            result.push_back(value);
            
        } catch (const std::exception& e) {
            LOG_WARNING("Failed to parse value: " + token + ", using NaN");
            result.push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    return result;
}

bool SensorData::hasSpecialValues() const {
    return std::isnan(distanceUpper1) || std::isinf(distanceUpper1) ||
           std::isnan(distanceUpper2) || std::isinf(distanceUpper2) ||
           std::isnan(distanceLower1) || std::isinf(distanceLower1) ||
           std::isnan(distanceLower2) || std::isinf(distanceLower2) ||
           std::isnan(temperature) || std::isinf(temperature) ||
           std::isnan(angle) || std::isinf(angle) ||
           std::isnan(capacitance) || std::isinf(capacitance);
}

std::string SensorData::getSpecialValuesDescription() const {
    std::vector<std::string> special;
    
    if (std::isnan(distanceUpper1)) special.push_back("D1:NaN");
    if (std::isinf(distanceUpper1)) special.push_back("D1:Inf");
    if (std::isnan(distanceUpper2)) special.push_back("D2:NaN");
    if (std::isinf(distanceUpper2)) special.push_back("D2:Inf");
    if (std::isnan(distanceLower1)) special.push_back("D3:NaN");
    if (std::isinf(distanceLower1)) special.push_back("D3:Inf");
    if (std::isnan(distanceLower2)) special.push_back("D4:NaN");
    if (std::isinf(distanceLower2)) special.push_back("D4:Inf");
    if (std::isnan(temperature)) special.push_back("Temp:NaN");
    if (std::isinf(temperature)) special.push_back("Temp:Inf");
    if (std::isnan(angle)) special.push_back("Angle:NaN");
    if (std::isinf(angle)) special.push_back("Angle:Inf");
    if (std::isnan(capacitance)) special.push_back("Cap:NaN");
    if (std::isinf(capacitance)) special.push_back("Cap:Inf");
    
    if (special.empty()) return "None";
    
    std::string result;
    for (size_t i = 0; i < special.size(); ++i) {
        if (i > 0) result += ", ";
        result += special[i];
    }
    return result;
}