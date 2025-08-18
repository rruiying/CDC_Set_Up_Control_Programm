#include "../include/sensor_interface.h"
#include "../include/serial_interface.h"
#include "../include/command_protocol.h"
#include "../../utils/include/logger.h"

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
    return m_serial->sendCommand("READ:DIST\r\n");
}

bool SensorInterface::requestAngleSensor() {
    if (!m_serial || !m_serial->isOpen()) {
        return false;
    }
    return m_serial->sendCommand("READ:ANGLE\r\n");
}

bool SensorInterface::requestTemperature() {
    if (!m_serial || !m_serial->isOpen()) {
        return false;
    }
    return m_serial->sendCommand("READ:TEMP\r\n");
}

bool SensorInterface::requestCapacitance() {
    if (!m_serial || !m_serial->isOpen()) {
        return false;
    }
    return m_serial->sendCommand("READ:CAP\r\n");
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
    std::lock_guard<std::mutex> lock(dataMutex);
    
    CommandResponse response = CommandProtocol::parseResponse(data);
    
    if (response.type == ResponseType::SENSOR_DATA && response.sensorData.has_value()) {
        m_latestData = response.sensorData.value();
        
        if (validateSensorData(m_latestData)) {
            if (dataCallback) {
                dataCallback(m_latestData);
            }
        } else {
            if (errorCallback) {
                errorCallback("Sensor data validation failed");
            }
        }
    }
}

SensorData SensorInterface::getLatestData() const {
    std::lock_guard<std::mutex> lock(dataMutex);
    return m_latestData;
}

void SensorInterface::setDataCallback(DataCallback callback) {
    dataCallback = callback;
}

void SensorInterface::setErrorCallback(ErrorCallback callback) {
    errorCallback = callback;
}

bool SensorInterface::validateSensorData(const SensorData& data) {
    if (data.distanceUpper1 < 0 || data.distanceUpper1 > 300) {
        return false;
    }
    
    if (data.temperature < -40 || data.temperature > 100) {
        return false;
    }
    
    return true;
}