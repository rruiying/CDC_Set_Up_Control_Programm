#include "../include/sensor_interface.h"
#include "../include/serial_interface.h"
#include "../include/command_protocol.h"
#include "../../utils/include/logger.h"
#include "../../models/include/physics_constants.h"
#include <cmath>

SensorInterface::SensorInterface(std::shared_ptr<SerialInterface> serial)
    : m_serial(serial) {
    LOG_INFO("SensorInterface initialized");
}

SensorInterface::~SensorInterface() {
    stopPolling();
}

bool SensorInterface::requestAllSensorData() {
    if (!m_serial || !m_serial->isOpen()) {
        return false;
    }
    
    std::string command = CommandProtocol::buildGetSensorsCommand();
    return m_serial->sendCommand(command);
}

bool SensorInterface::requestDistanceSensors() {
    if (!m_serial || !m_serial->isOpen()) {
        return false;
    }
    return m_serial->sendCommand(CommandProtocol::buildCustomCommand("READ", "DIST"));
}

bool SensorInterface::requestAngleSensor() {
    if (!m_serial || !m_serial->isOpen()) {
        return false;
    }
    return m_serial->sendCommand(CommandProtocol::buildCustomCommand("READ", "ANGLE"));
}

bool SensorInterface::requestTemperature() {
    if (!m_serial || !m_serial->isOpen()) {
        return false;
    }
    return m_serial->sendCommand(CommandProtocol::buildCustomCommand("READ", "TEMP"));
}

bool SensorInterface::requestCapacitance() {
    if (!m_serial || !m_serial->isOpen()) {
        return false;
    }
    return m_serial->sendCommand(CommandProtocol::buildCustomCommand("READ", "CAP"));
}

void SensorInterface::startPolling(int intervalMs) {
    if (polling) return;
    
    polling = true;
    stopPollingFlag = false;
    pollInterval = intervalMs;
    
    m_pollThread = std::make_unique<std::thread>(&SensorInterface::pollThread, this);
    LOG_INFO_F("Started sensor polling at %dms interval", intervalMs);
}

void SensorInterface::stopPolling() {
    if (!polling) return;
    
    stopPollingFlag = true;
    
    if (m_pollThread && m_pollThread->joinable()) {
        m_pollThread->join();
    }
    
    polling = false;
    LOG_INFO("Stopped sensor polling");
}

void SensorInterface::pollThread() {
    while (!stopPollingFlag) {
        if (requestAllSensorData()) {
            std::string response = m_serial->readLine(1000);
            if (!response.empty()) {
                processData(response);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(pollInterval.load()));
    }
}

void SensorInterface::processData(const std::string& data) {  
    CommandResponse response = CommandProtocol::parseResponse(data);
    
    if (response.type != ResponseType::SENSOR_DATA || 
        !response.sensorData.has_value()) {
        return;
    }
    
    SensorData sensorDataCopy = response.sensorData.value();
    
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        m_latestData = sensorDataCopy;
    }
    
    if (validateSensorData(sensorDataCopy)) {
        if (dataCallback) {
            dataCallback(sensorDataCopy);
        }
    } else {
        if (errorCallback) {
            errorCallback("Sensor data validation failed");
        }
    }
}

SensorData SensorInterface::getLatestData() const {
    std::lock_guard<std::mutex> lock(dataMutex);
    return m_latestData;
}

void SensorInterface::setDataCallback(DataCallback callback) {
    std::lock_guard<std::mutex> lock(dataMutex);
    dataCallback = std::move(callback);
}

void SensorInterface::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(dataMutex);
    errorCallback = std::move(callback);
}

bool SensorInterface::validateSensorData(const SensorData& data) {
    if (data.distanceUpper1 < PhysicsConstants::DISTANCE_SENSOR_MIN || 
        data.distanceUpper1 > PhysicsConstants::DISTANCE_SENSOR_MAX) {
        return false;
    }
    
    if (data.temperature < PhysicsConstants::TEMPERATURE_MIN || 
        data.temperature > PhysicsConstants::TEMPERATURE_MAX) {
        return false;
    }
    
    auto finite = [](double v){ return std::isfinite(v); };
    if (!finite(data.distanceUpper1) || !finite(data.distanceUpper2) || 
        !finite(data.temperature) || !finite(data.angle) ||
        !finite(data.capacitance)) {
        return false;
    }
    return true;
}