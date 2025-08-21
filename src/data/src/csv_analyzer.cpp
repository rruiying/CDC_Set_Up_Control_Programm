#include "../include/csv_analyzer.h"
#include "../../utils/include/logger.h"
#include "../../models/include/system_config.h"
#include "../include/data_processor.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <set>


class CsvAnalyzer::Impl {
public:
    std::vector<MeasurementData> data;
    mutable DataStatistics cachedStats;
    mutable bool statsValid = false;
    mutable bool errorStatsValid = false;
};

CsvAnalyzer::CsvAnalyzer() : pImpl(std::make_unique<Impl>()) {
    LOG_INFO("CsvAnalyzer initialized");
}

CsvAnalyzer::~CsvAnalyzer() = default;


bool CsvAnalyzer::loadCsvFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open CSV file: " + filename);
        return false;
    }
    
    pImpl->data.clear();
    pImpl->statsValid = false;
    pImpl->errorStatsValid = false;
    
    std::string line;

    std::getline(file, line);
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string field;
        
        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }
        
        if (fields.size() >= 16) { 
            try {
                double setHeight = std::stod(fields[1]);
                double setAngle = std::stod(fields[2]);
                
                // 创建SensorData
                SensorData sensorData;
                sensorData.setUpperSensors(std::stod(fields[4]), std::stod(fields[5]));
                sensorData.setLowerSensors(std::stod(fields[6]), std::stod(fields[7]));
                sensorData.setTemperature(std::stod(fields[8]));
                sensorData.setAngle(std::stod(fields[9]));
                sensorData.setCapacitance(std::stod(fields[10]));

                MeasurementData measurement(setHeight, setAngle, sensorData);
                pImpl->data.push_back(measurement);
                
            } catch (const std::exception& e) {
                LOG_WARNING("Failed to parse CSV line: " + std::string(e.what()));
            }
        }
    }
    
    file.close();
    LOG_INFO_F("Loaded %zu measurements from CSV", pImpl->data.size());
    return !pImpl->data.empty();
}

void CsvAnalyzer::setData(const std::vector<MeasurementData>& data) {
    pImpl->data = data;
    pImpl->statsValid = false;
    pImpl->errorStatsValid = false;
}

void CsvAnalyzer::clearData() {
    pImpl->data.clear();
    pImpl->statsValid = false;
    pImpl->errorStatsValid = false;
}

std::vector<MeasurementData> CsvAnalyzer::getData() const {
    return pImpl->data;
}

size_t CsvAnalyzer::getDataCount() const {
    return pImpl->data.size();
}

bool CsvAnalyzer::saveCsvFile(const std::string& filename, 
                              const std::vector<MeasurementData>& data) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open CSV file for writing: " + filename);
        return false;
    }
    
    file << MeasurementData::getCSVHeader() << std::endl;
    
    for (const auto& measurement : data) {
        file << measurement.toCSV() << std::endl;
    }
    
    file.close();
    LOG_INFO_F("Saved %zu measurements to CSV", data.size());
    return true;
}
