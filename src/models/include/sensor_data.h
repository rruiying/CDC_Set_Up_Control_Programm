#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

class SensorData {
public:
SensorData();
    SensorData(const SensorData& other);
    SensorData& operator=(const SensorData& other);
    
    double distanceUpper1;  
    double distanceUpper2;  
    double distanceLower1;   
    double distanceLower2;   
    double temperature;
    double angle;
    double capacitance;
    
    int64_t timestamp;  
    
    struct ValidityFlags {
        bool distanceUpper1;
        bool distanceUpper2;
        bool distanceLower1;
        bool distanceLower2;
        bool temperature;
        bool angle;
        bool capacitance;
    } isValid;
    
    double getAverageHeight() const;
    double getCalculatedAngle() const;
    double getAverageGroundDistance() const;
    double getCalculatedUpperDistance() const;
    
    bool setUpperSensors(double sensor1, double sensor2);
    bool setLowerSensors(double sensor1, double sensor2);
    bool setTemperature(double temp);
    bool setAngle(double angle);
    bool setCapacitance(double cap);
    
    bool isAllValid() const;
    bool hasValidData() const;
    
    bool parseFromString(const std::string& dataString);
    std::string toString() const;
    std::string toCSV() const;
    static std::string getCSVHeader();
    
    void reset();
    
private:
    std::vector<double> parseNumbers(const std::string& str) const;
};

#endif // SENSOR_DATA_H