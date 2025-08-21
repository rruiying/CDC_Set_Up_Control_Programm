#include "../include/application_controller.h"
#include "../../hardware/include/serial_interface.h"
#include "../../core/include/motor_controller.h"
#include "../../core/include/sensor_manager.h"
#include "../../core/include/safety_manager.h"
#include "../../core/include/data_recorder.h"
#include "../../data/include/export_manager.h"
#include "../../models/include/device_info.h"
#include "../../models/include/sensor_data.h"
#include "../../models/include/measurement_data.h"
#include "../../utils/include/logger.h"
#include "../../utils/include/math_utils.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <fstream>
#include <ctime>
#include <thread>
#include <atomic>

// 实现结构体
struct ApplicationController::Impl {
    std::shared_ptr<SerialInterface> serial;
    std::shared_ptr<MotorController> motor;
    std::shared_ptr<SensorManager> sensor;
    std::shared_ptr<SafetyManager> safety;
    std::shared_ptr<DataRecorder> recorder;
    std::shared_ptr<ExportManager> exporter;

    std::thread serialReadThread;
    std::atomic<bool> isReading{false};
    
    std::vector<DeviceInfo> devices;
    int currentDeviceIndex = -1;
    
    ConnectionCallback connectionCallback;
    DataCallback dataCallback;
    SensorCallback sensorCallback;
    MotorCallback motorCallback;
    ErrorCallback errorCallback;
    ProgressCallback progressCallback;
    
    // 当前状态缓存
    mutable std::mutex stateMutex;
    SensorData lastSensorData;
    double targetHeight = 0.0;
    double targetAngle = 0.0;
    
    // 工具方法
    std::string sensorDataToJson(const SensorData& data) const;
    std::string measurementToJson(const MeasurementData& data) const;
    std::string deviceInfoToJson(const DeviceInfo& device) const;
    std::string deviceListToJson(const std::vector<DeviceInfo>& devices) const {
    std::stringstream json;
    json << "[";
    for (size_t i = 0; i < devices.size(); ++i) {
        if (i > 0) json << ",";
        json << deviceInfoToJson(devices[i]);
    }
    json << "]";
    return json.str();
    }
    void setupCallbacks();

    void startSerialReading() {
        Logger::getInstance().warning("startSerialReading() called");

        if (isReading) {
            Logger::getInstance().warning("Already reading, skipping");
            return;
        }
        
        isReading = true;
        Logger::getInstance().warning("Starting read thread...");

        serialReadThread = std::thread([this]() {
            Logger::getInstance().info("Starting serial read thread");
            
            while (isReading && serial && serial->isOpen()) {
                int available = serial->bytesAvailable();
                if (available > 0) {
                    Logger::getInstance().warning("!!! " + std::to_string(available) + " bytes available !!!");
                    auto bytes = serial->readBytes(available, 100);
                    if (!bytes.empty()) {
                        std::string data(bytes.begin(), bytes.end());
                        Logger::getInstance().warning("!!! RAW DATA: [" + data + "] !!!");
                    
                        std::string hex;
                        for (uint8_t b : bytes) {
                            char buf[8];
                            sprintf(buf, "%02X ", b);
                            hex += buf;
                        }
                        Logger::getInstance().warning("!!! HEX: " + hex + " !!!");
                    
                        if (dataCallback) {
                            dataCallback(data);
                        }
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            Logger::getInstance().info("Serial read thread stopped");
        });
        Logger::getInstance().warning("Read thread launched");
    }
    
    void stopSerialReading() {
        isReading = false;
        if (serialReadThread.joinable()) {
            serialReadThread.join();
        }
    }
    
    ~Impl() {
        stopSerialReading();
    }
};

// 构造函数和析构函数
ApplicationController::ApplicationController() 
    : pImpl(std::make_unique<Impl>()) {
}

ApplicationController::~ApplicationController() = default;

// 初始化
void ApplicationController::initialize(
    std::shared_ptr<SerialInterface> serial,
    std::shared_ptr<MotorController> motor,
    std::shared_ptr<SensorManager> sensor,
    std::shared_ptr<SafetyManager> safety,
    std::shared_ptr<DataRecorder> recorder,
    std::shared_ptr<ExportManager> exporter) {

    if (!serial) {
        Logger::getInstance().error("SerialInterface is null in initialize!");
        throw std::runtime_error("SerialInterface cannot be null");
    }
    
    pImpl->serial = serial;
    pImpl->motor = motor;
    pImpl->sensor = sensor;
    pImpl->safety = safety;
    pImpl->recorder = recorder;
    pImpl->exporter = exporter;
    
    pImpl->setupCallbacks();

     Logger::getInstance().info("ApplicationController initialized successfully");
}

// ===== 设备管理实现 =====

bool ApplicationController::addDevice(const std::string& name, 
                                     const std::string& port, 
                                     int baudRate) {
    // 检查端口是否已被使用
    for (const auto& device : pImpl->devices) {
        if (device.getPortName() == port && device.isConnected()) {
            if (pImpl->errorCallback) {
                pImpl->errorCallback("Port " + port + " is already in use");
            }
            return false;
        }
    }
    
    DeviceInfo newDevice(name, port, baudRate);
    pImpl->devices.push_back(newDevice);
    
    Logger::getInstance().info("Device added: " + name + " on " + port);
    return true;
}

bool ApplicationController::removeDevice(size_t index) {
    if (index >= pImpl->devices.size()) {
        return false;
    }
    
    if (static_cast<int>(index) == pImpl->currentDeviceIndex) {
        disconnectDevice(index);
    }
    
    pImpl->devices.erase(pImpl->devices.begin() + index);
    
    if (pImpl->currentDeviceIndex >= static_cast<int>(index)) {
        pImpl->currentDeviceIndex--;
    }
    
    return true;
}

bool ApplicationController::connectDevice(size_t index) {
    Logger::getInstance().info("Attempting to connect device at index: " + std::to_string(index));
    
    if (index >= pImpl->devices.size() || !pImpl->serial) {
        return false;
    }
    
    auto& device = pImpl->devices[index];
    
    if (pImpl->serial->isOpen()) {
        pImpl->stopSerialReading();
        pImpl->serial->close();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    for (auto& d : pImpl->devices) {
        if (d.isConnected()) {
            d.setConnectionStatus(ConnectionStatus::DISCONNECTED);
        }
    }
    
    device.setConnectionStatus(ConnectionStatus::CONNECTING);
    
    Logger::getInstance().info("Opening port: " + device.getPortName() + " at " + std::to_string(device.getBaudRate()));
    
    bool success = false;
    
    for (int retry = 0; retry < 3; retry++) {
        try {
            success = pImpl->serial->open(device.getPortName(), device.getBaudRate());
            if (success) break;
        } catch (const std::exception& e) {
            Logger::getInstance().error("Attempt " + std::to_string(retry + 1) + " failed: " + e.what());
        }
        
        if (!success && retry < 2) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    if (success) {
        device.setConnectionStatus(ConnectionStatus::CONNECTED);
        pImpl->currentDeviceIndex = index;

        pImpl->startSerialReading();
        Logger::getInstance().warning("!!! Started serial reading thread !!!");

        if (pImpl->connectionCallback) {
            pImpl->connectionCallback(true, device.getName());
        }
        
        Logger::getInstance().info("Device connected successfully: " + device.getName());
    } else {
        device.setConnectionStatus(ConnectionStatus::ERROR);
        device.recordError("Failed to connect after 3 attempts");
        
        if (pImpl->errorCallback) {
            pImpl->errorCallback("Failed to connect to " + device.getName());
        }
    }
    
    return success;
}

bool ApplicationController::disconnectDevice(size_t index) {
    if (index >= pImpl->devices.size()) {
        return false;
    }
    
    auto& device = pImpl->devices[index];
    
    if (pImpl->serial && pImpl->serial->isOpen()) {
        pImpl->stopSerialReading();

        if (pImpl->sensor) {
            pImpl->sensor->stop();
        }
        
        pImpl->serial->close();
    }
    
    device.setConnectionStatus(ConnectionStatus::DISCONNECTED);
    pImpl->currentDeviceIndex = -1;
    
    if (pImpl->connectionCallback) {
        pImpl->connectionCallback(false, device.getName());
    }
    
    Logger::getInstance().info("Device disconnected: " + device.getName());
    return true;
}

bool ApplicationController::sendCommand(const std::string& command) {
    if (!pImpl->serial || !pImpl->serial->isOpen()) {
        return false;
    }
    
    return pImpl->serial->sendCommand(command);
}

std::vector<DeviceInfoData> ApplicationController::getDeviceList() const {
    std::vector<DeviceInfoData> result;
    
    for (const auto& device : pImpl->devices) {
        DeviceInfoData data;
        data.name = device.getName();
        data.portName = device.getPortName();
        data.baudRate = device.getBaudRate();
        data.connectionStatus = static_cast<int>(device.getConnectionStatus());
        data.deviceType = device.getDeviceTypeString();
        data.errorCount = device.getErrorCount();
        data.lastError = device.getLastErrorMessage();
        
        result.push_back(data);
    }
    
    return result;
}

std::vector<std::string> ApplicationController::getAvailablePorts() const {
    std::vector<std::string> ports;
    auto portInfos = SerialInterface::getAvailablePorts();
    
    for (const auto& info : portInfos) {
        ports.push_back(info.portName);
    }
    
    return ports;
}

// ===== 电机控制实现 =====

bool ApplicationController::moveToPosition(double height, double angle) {
    if (!pImpl->motor) {
        return false;
    }
    
    // 安全检查
    if (pImpl->safety && !pImpl->safety->checkPosition(height, angle)) {
        if (pImpl->errorCallback) {
            pImpl->errorCallback("Position exceeds safety limits");
        }
        return false;
    }
    
    pImpl->targetHeight = height;
    pImpl->targetAngle = angle;
    
    bool result = pImpl->motor->moveToPosition(height, angle);
    
    if (result) {
        Logger::getInstance().infof("Moving to position: %.1fmm, %.1f°", height, angle);
    }
    
    return result;
}

bool ApplicationController::homeMotor() {
    if (!pImpl->motor) {
        return false;
    }
    
    bool result = pImpl->motor->home();
    
    if (result) {
        pImpl->targetHeight = 0.0;
        pImpl->targetAngle = 0.0;
        Logger::getInstance().info("Motor homing initiated");
    }
    
    return result;
}

bool ApplicationController::stopMotor() {
    if (!pImpl->motor) {
        return false;
    }
    
    return pImpl->motor->stop();
}

bool ApplicationController::emergencyStop() {
    bool result = false;
    
    // 触发安全管理器的紧急停止
    if (pImpl->safety) {
        pImpl->safety->triggerEmergencyStop("User activated");
    }
    
    // 停止电机
    if (pImpl->motor) {
        result = pImpl->motor->emergencyStop();
    }
    
    // 直接发送紧急停止命令到串口
    if (pImpl->serial && pImpl->serial->isOpen()) {
        pImpl->serial->sendCommand("EMERGENCY_STOP\r\n");
    }
    
    Logger::getInstance().error("EMERGENCY STOP ACTIVATED");
    
    if (pImpl->errorCallback) {
        pImpl->errorCallback("Emergency stop activated");
    }
    
    return result;
}

void ApplicationController::setSafetyLimits(double minHeight, double maxHeight,
                                           double minAngle, double maxAngle) {
    if (pImpl->safety) {
        pImpl->safety->setCustomLimits(minHeight, maxHeight, minAngle, maxAngle);
        Logger::getInstance().infof("Safety limits updated: H[%.1f-%.1f]mm, A[%.1f-%.1f]°",
                                   minHeight, maxHeight, minAngle, maxAngle);
    }
}

bool ApplicationController::checkSafetyLimits(double height, double angle) const {
    if (!pImpl->safety) {
        return true;  // 如果没有安全管理器，默认通过
    }
    
    return pImpl->safety->checkPosition(height, angle);
}

double ApplicationController::getCurrentHeight() const {
    if (pImpl->motor) {
        return pImpl->motor->getCurrentHeight();
    }
    return 0.0;
}

double ApplicationController::getCurrentAngle() const {
    if (pImpl->motor) {
        return pImpl->motor->getCurrentAngle();
    }
    return 0.0;
}

int ApplicationController::getMotorStatus() const {
    if (pImpl->motor) {
        return static_cast<int>(pImpl->motor->getStatus());
    }
    return 0;  // IDLE
}

// ===== 传感器监控实现 =====

bool ApplicationController::startSensorMonitoring() {
    if (!pImpl->sensor) {
        return false;
    }
    
    return pImpl->sensor->start();
}

bool ApplicationController::stopSensorMonitoring() {
    if (!pImpl->sensor) {
        return false;
    }
    
    pImpl->sensor->stop();
    return true;
}

bool ApplicationController::recordCurrentData() {
    if (!pImpl->sensor || !pImpl->recorder) {
        return false;
    }
    
    if (!pImpl->sensor->hasValidData()) {
        if (pImpl->errorCallback) {
            pImpl->errorCallback("No valid sensor data available");
        }
        return false;
    }
    
    SensorData sensorData = pImpl->sensor->getLatestData();
    double height = getCurrentHeight();
    double angle = getCurrentAngle();
    
    MeasurementData measurement(height, angle, sensorData);
    pImpl->recorder->addMeasurement(measurement);
    
    Logger::getInstance().infof("Data recorded: H=%.1fmm, A=%.1f°", height, angle);
    return true;
}

size_t ApplicationController::getRecordCount() const {
    if (pImpl->recorder) {
        return pImpl->recorder->getRecordCount();
    }
    return 0;
}

std::string ApplicationController::getCurrentSensorDataJson() const {
    std::lock_guard<std::mutex> lock(pImpl->stateMutex);
    
    if (!pImpl->sensor || !pImpl->sensor->hasValidData()) {
        return "{}";
    }
    
    SensorData data = pImpl->sensor->getLatestData();
    return pImpl->sensorDataToJson(data);
}

// ===== 数据导出实现 =====

bool ApplicationController::exportToCSV(const std::string& filename) {
    if (!pImpl->recorder) {
        Logger::getInstance().error("No recorder available");
        return false;
    }
    return pImpl->recorder->exportToCSV(filename);
}

std::vector<MeasurementData> ApplicationController::getRecordedData() const {
    std::vector<MeasurementData> result;
    
    if (pImpl->recorder) {
        int count = pImpl->recorder->getRecordCount();
        
        if (count > 0) {
            result.push_back(pImpl->recorder->getLatestMeasurement());
        }
        
        Logger::getInstance().info("Returning " + std::to_string(result.size()) + " records");
    }
    
    return result;
}

std::string ApplicationController::generateDefaultFilename() const {
    if (pImpl->recorder) {
        return pImpl->recorder->getDefaultFilename();
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "CDC_Data_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".csv";
    return ss.str();
}

// ===== 日志实现 =====

void ApplicationController::logOperation(const std::string& operation) {
    Logger::getInstance().info(operation, "Operation");
}

void ApplicationController::logError(const std::string& error) {
    Logger::getInstance().error(error);
}

void ApplicationController::logWarning(const std::string& warning) {
    Logger::getInstance().warning(warning);
}

void ApplicationController::logInfo(const std::string& info) {
    Logger::getInstance().info(info);
}

std::string ApplicationController::getRecentLogsJson(int count) const {
    auto logs = Logger::getInstance().getRecentLogs(count);
    
    std::stringstream json;
    json << "[";
    
    for (size_t i = 0; i < logs.size(); ++i) {
        if (i > 0) json << ",";
        
        auto time_t = std::chrono::system_clock::to_time_t(logs[i].timestamp);
        
        json << "{"
             << "\"time\":\"" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\","
             << "\"level\":" << static_cast<int>(logs[i].level) << ","
             << "\"category\":\"" << logs[i].category << "\","
             << "\"message\":\"" << logs[i].message << "\""
             << "}";
    }
    
    json << "]";
    return json.str();
}

// ===== 回调设置 =====

void ApplicationController::setConnectionCallback(ConnectionCallback cb) {
    pImpl->connectionCallback = cb;
}

void ApplicationController::setDataCallback(DataCallback cb) {
    pImpl->dataCallback = cb;
}

void ApplicationController::setSensorCallback(SensorCallback cb) {
    pImpl->sensorCallback = cb;
}

void ApplicationController::setMotorCallback(MotorCallback cb) {
    pImpl->motorCallback = cb;
}

void ApplicationController::setErrorCallback(ErrorCallback cb) {
    pImpl->errorCallback = cb;
}

void ApplicationController::setProgressCallback(ProgressCallback cb) {
    pImpl->progressCallback = cb;
}

// ===== 内部实现方法 =====

void ApplicationController::Impl::setupCallbacks() {
    // 设置串口回调
    if (serial) {
        serial->setConnectionCallback([this](bool connected) {
            if (connectionCallback && currentDeviceIndex >= 0) {
                connectionCallback(connected, devices[currentDeviceIndex].getName());
            }
        });
        
        serial->setDataReceivedCallback([this](const std::string& data) {
            if (dataCallback) {
                dataCallback(data);
            }
        });
        
        serial->setErrorCallback([this](const std::string& error) {
            if (errorCallback) {
                errorCallback(error);
            }
        });
    }
    
    // 设置电机回调
    if (motor) {
        motor->setStatusCallback([this](MotorStatus status) {
            if (motorCallback) {
                motorCallback(static_cast<int>(status));
            }
        });
        
        motor->setErrorCallback([this](const MotorError& error) {
            if (errorCallback) {
                errorCallback(error.message);
            }
        });
    }
    
    // 设置传感器回调
    if (sensor) {
        sensor->setDataCallback([this](const SensorData& data) {
            std::lock_guard<std::mutex> lock(stateMutex);
            lastSensorData = data;
            
            if (sensorCallback) {
                sensorCallback(sensorDataToJson(data));
            }
        });
        
        sensor->setErrorCallback([this](const std::string& error) {
            if (errorCallback) {
                errorCallback("Sensor: " + error);
            }
        });
    }
    
    // 设置导出进度回调
    if (exporter) {
        exporter->setProgressCallback([this](int percentage) {
            if (progressCallback) {
                progressCallback(percentage);
            }
        });
    }
}

std::string ApplicationController::Impl::sensorDataToJson(const SensorData& data) const {
    std::stringstream json;
    json << "{"
         << "\"height\":" << data.getAverageHeight() << ","
         << "\"angle\":" << data.angle << ","
         << "\"temperature\":" << data.temperature << ","
         << "\"capacitance\":" << data.capacitance << ","
         << "\"distanceUpper1\":" << data.distanceUpper1 << ","
         << "\"distanceUpper2\":" << data.distanceUpper2 << ","
         << "\"distanceLower1\":" << data.distanceLower1 << ","
         << "\"distanceLower2\":" << data.distanceLower2 << ","
         << "\"timestamp\":" << data.timestamp << ","
         << "\"valid\":" << (data.isAllValid() ? "true" : "false")
         << "}";
    return json.str();
}

DeviceInfoData ApplicationController::getCurrentDevice() const {
    if (pImpl->currentDeviceIndex >= 0 && 
        pImpl->currentDeviceIndex < pImpl->devices.size()) {
        
        auto& device = pImpl->devices[pImpl->currentDeviceIndex];
        DeviceInfoData data;
        data.name = device.getName();
        data.portName = device.getPortName();
        data.baudRate = device.getBaudRate();
        data.connectionStatus = static_cast<int>(device.getConnectionStatus());
        data.deviceType = device.getDeviceTypeString();
        data.errorCount = device.getErrorCount();
        data.lastError = device.getLastErrorMessage();
        return data;
    }
    
    DeviceInfoData empty;
    empty.connectionStatus = 0;
    return empty;
}

bool ApplicationController::setTargetHeight(double height) {
    if (!pImpl->motor) return false;
    
    pImpl->targetHeight = height;
    Logger::getInstance().infof("Target height set to %.1f mm", height);
    return true;
}

bool ApplicationController::setTargetAngle(double angle) {
    if (!pImpl->motor) return false;
    
    pImpl->targetAngle = angle;
    Logger::getInstance().infof("Target angle set to %.1f°", angle);
    return true;
}
bool ApplicationController::pauseSensorMonitoring() {
    if (!pImpl->sensor) return false;
    
    pImpl->sensor->pause();
    Logger::getInstance().info("Sensor monitoring paused");
    return true;
}

bool ApplicationController::resumeSensorMonitoring() {
    if (!pImpl->sensor) return false;
    
    pImpl->sensor->resume();
    Logger::getInstance().info("Sensor monitoring resumed");
    return true;
}

void ApplicationController::clearRecords() {
    if (pImpl->recorder) {
        pImpl->recorder->clear();
        Logger::getInstance().info("All records cleared");
    }
}

double ApplicationController::getMaxHeight() const {
    return SystemConfig::getInstance().getMaxHeight();
}

double ApplicationController::getMinHeight() const {
    return SystemConfig::getInstance().getMinHeight();
}

double ApplicationController::getMaxAngle() const {
    return SystemConfig::getInstance().getMaxAngle();
}

double ApplicationController::getMinAngle() const {
    return SystemConfig::getInstance().getMinAngle();
}

std::string ApplicationController::Impl::deviceInfoToJson(const DeviceInfo& device) const {
    std::stringstream json;
    json << "{"
         << "\"name\":\"" << device.getName() << "\","
         << "\"port\":\"" << device.getPortName() << "\","
         << "\"baudRate\":" << device.getBaudRate() << ","
         << "\"status\":" << static_cast<int>(device.getConnectionStatus()) << ","
         << "\"type\":\"" << device.getDeviceTypeString() << "\","
         << "\"errors\":" << device.getErrorCount()
         << "}";
    return json.str();
}

std::string ApplicationController::Impl::measurementToJson(const MeasurementData& data) const {
    std::stringstream json;
    json << "{"
         << "\"timestamp\":" << data.getTimestamp() << ","
         << "\"setHeight\":" << data.getSetHeight() << ","
         << "\"setAngle\":" << data.getSetAngle() << ","
         << "\"measuredHeight\":" << data.getSensorData().getAverageHeight() << ","
         << "\"measuredAngle\":" << data.getSensorData().angle << ","
         << "\"temperature\":" << data.getSensorData().temperature << ","
         << "\"capacitance\":" << data.getSensorData().capacitance
         << "}";
    return json.str();
}

namespace {
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
}


bool ApplicationController::isPortInUse(const std::string& port) const {
    for (const auto& device : pImpl->devices) {
        if (device.getPortName() == port && device.isConnected()) {
            return true;
        }
    }
    return false;
}

double ApplicationController::calculateTheoreticalCapacitance(double height, double angle) const {
    const double BASE_CAPACITANCE = 10.0;  // pF
    const double HEIGHT_FACTOR = 0.5;      // pF/mm
    const double ANGLE_FACTOR = 0.1;       // pF/degree
    
    return BASE_CAPACITANCE + (height * HEIGHT_FACTOR) + (std::abs(angle) * ANGLE_FACTOR);
}

void ApplicationController::clearLogs() {
    Logger::getInstance().clear();
    logOperation("Logs cleared");
}

bool ApplicationController::saveLogsToFile(const std::string& filename) {
    try {
        // 获取所有日志
        auto logs = Logger::getInstance().getAllLogs();
        
        // 打开文件
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        // 写入日志
        file << "CDC Control System Log File\n";
        file << "Generated: " << getCurrentTimestamp() << "\n";
        file << "=====================================\n\n";
        
        for (const auto& log : logs) {
            file << log << "\n";
        }
        
        file.close();
        logOperation("Logs saved to file: " + filename);
        return true;
    } catch (...) {
        return false;
    }
}
