// src/core/src/sensor_manager.cpp
#include "../include/sensor_manager.h"
#include "../../hardware/include/serial_interface.h"
#include "../../hardware/include/command_protocol.h"
#include "../../models/include/system_config.h"
#include "../../utils/include/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>

SensorManager::SensorManager(std::shared_ptr<SerialInterface> serialInterface)
    : serial(serialInterface) {
    // 从系统配置获取默认更新间隔
    updateInterval = SystemConfig::getInstance().getSensorUpdateInterval();
    
    LOG_INFO("SensorManager initialized with update interval: " + std::to_string(updateInterval.load()) + "ms");
}

SensorManager::~SensorManager() {
    stop();
}

bool SensorManager::start() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (running) {
        LOG_WARNING("SensorManager already running");
        return false;
    }
    
    if (!serial || !serial->isOpen()) {
        LOG_ERROR("Cannot start SensorManager: serial port not open");
        return false;
    }
    
    running = true;
    stopRequested = false;
    paused = false;
    
    // 启动更新线程
    updateThreadPtr = std::make_unique<std::thread>(&SensorManager::updateThread, this);
    
    LOG_INFO("SensorManager started");
    return true;
}

void SensorManager::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) {
            return;
        }
        
        stopRequested = true;
        cv.notify_all();
    }
    
    // 等待线程结束
    if (updateThreadPtr && updateThreadPtr->joinable()) {
        updateThreadPtr->join();
    }
    
    running = false;
    LOG_INFO("SensorManager stopped");
}

void SensorManager::pause() {
    std::lock_guard<std::mutex> lock(mutex);
    paused = true;
    LOG_INFO("SensorManager paused");
}

void SensorManager::resume() {
    std::lock_guard<std::mutex> lock(mutex);
    paused = false;
    cv.notify_all();
    LOG_INFO("SensorManager resumed");
}

bool SensorManager::readSensorsOnce() {
    auto startTime = getCurrentTimestamp();
    bool success = performRead();
    auto readTime = getCurrentTimestamp() - startTime;
    
    updateStatistics(success, readTime);
    
    return success;
}

bool SensorManager::hasValidData() const {
    std::lock_guard<std::mutex> lock(mutex);
    return hasData && latestData.isAllValid();
}

SensorData SensorManager::getLatestData() const {
    std::lock_guard<std::mutex> lock(mutex);
    return latestData;
}

std::vector<SensorData> SensorManager::getDataHistory() const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::vector<SensorData>(dataHistory.begin(), dataHistory.end());
}

SensorData SensorManager::getAverageData(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (dataHistory.empty()) {
        return SensorData();
    }
    
    // 限制计数
    count = std::min(count, dataHistory.size());
    
    // 计算平均值
    double sumUpper1 = 0, sumUpper2 = 0;
    double sumLower1 = 0, sumLower2 = 0;
    double sumTemp = 0, sumAngle = 0, sumCap = 0;
    
    auto it = dataHistory.rbegin();
    for (size_t i = 0; i < count && it != dataHistory.rend(); ++i, ++it) {
        sumUpper1 += it->distanceUpper1;
        sumUpper2 += it->distanceUpper2;
        sumLower1 += it->distanceLower1;
        sumLower2 += it->distanceLower2;
        sumTemp += it->temperature;
        sumAngle += it->angle;
        sumCap += it->capacitance;
    }
    
    // 创建平均数据
    SensorData avgData;
    avgData.setUpperSensors(sumUpper1 / count, sumUpper2 / count);
    avgData.setLowerSensors(sumLower1 / count, sumLower2 / count);
    avgData.setTemperature(sumTemp / count);
    avgData.setAngle(sumAngle / count);
    avgData.setCapacitance(sumCap / count);
    
    return avgData;
}

void SensorManager::setUpdateInterval(int intervalMs) {
    updateInterval = intervalMs;
    cv.notify_all(); // 通知线程更新间隔已改变
    LOG_INFO_F("Update interval set to %d ms", intervalMs);
}

void SensorManager::setHistorySize(size_t size) {
    std::lock_guard<std::mutex> lock(mutex);
    maxHistorySize = size;
    
    // 如果当前历史超过新大小，删除旧数据
    while (dataHistory.size() > maxHistorySize) {
        dataHistory.pop_front();
    }
}

void SensorManager::setDataCallback(DataCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    dataCallback = callback;
}

void SensorManager::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    errorCallback = callback;
}

SensorStatistics SensorManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 计算成功率
    if (statistics.totalReads > 0) {
        statistics.successRate = (double)statistics.successfulReads / statistics.totalReads * 100.0;
        statistics.averageReadTime = (double)statistics.totalReadTime / statistics.totalReads;
    }
    
    return statistics;
}

void SensorManager::resetStatistics() {
    std::lock_guard<std::mutex> lock(mutex);
    statistics = SensorStatistics();
}

void SensorManager::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    
    hasData = false;
    latestData = SensorData();
    dataHistory.clear();
    statistics = SensorStatistics();
    
    LOG_INFO("SensorManager reset");
}

// 私有方法实现

void SensorManager::updateThread() {
    LOG_INFO("Sensor update thread started");
    
    while (!stopRequested) {
        std::unique_lock<std::mutex> lock(mutex);
        
        // 等待更新间隔或停止信号
        cv.wait_for(lock, std::chrono::milliseconds(updateInterval.load()),
                    [this] { return stopRequested.load(); });
        
        if (stopRequested) {
            break;
        }
        
        if (paused) {
            continue;
        }
        
        lock.unlock();
        
        // 执行读取
        readSensorsOnce();
    }
    
    LOG_INFO("Sensor update thread stopped");
}

bool SensorManager::performRead() {
    if (!serial || !serial->isOpen()) {
        notifyError("Serial port not open");
        return false;
    }
    
    try {
        // 发送获取传感器命令
        std::string command = CommandProtocol::buildGetSensorsCommand();
        std::string response = serial->sendAndReceive(command, readTimeout);
        
        if (response.empty()) {
            notifyError("Timeout reading sensors");
            return false;
        }
        
        // 解析响应
        CommandResponse cmdResponse = CommandProtocol::parseResponse(response);
        
        if (cmdResponse.type == ResponseType::SENSOR_DATA && cmdResponse.sensorData.has_value()) {
            SensorData newData = cmdResponse.sensorData.value();
            
            // 验证数据
            if (!isDataValid(newData)) {
                notifyError("Invalid sensor data received");
                return false;
            }
            
            // 检查是否需要过滤
            if (filteringEnabled && hasData && shouldFilterData(newData)) {
                LOG_WARNING("Sensor data filtered due to large change");
                notifyError("Sensor data filtered");
                return false;
            }
            
            // 处理新数据
            processNewData(newData);
            return true;
            
        } else if (cmdResponse.type == ResponseType::ERROR) {
            notifyError("Sensor error: " + cmdResponse.errorMessage);
            return false;
        } else {
            notifyError("Unexpected response type");
            return false;
        }
        
    } catch (const std::exception& e) {
        notifyError(std::string("Exception reading sensors: ") + e.what());
        return false;
    }
}

void SensorManager::processNewData(const SensorData& data) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 更新最新数据
    latestData = data;
    hasData = true;
    
    // 添加到历史
    dataHistory.push_back(data);
    
    // 限制历史大小
    while (dataHistory.size() > maxHistorySize) {
        dataHistory.pop_front();
    }
    
    // 通知回调
    notifyDataReceived(data);
    
    LOG_INFO_F("Sensor data updated: Upper[%.1f,%.1f] Lower[%.1f,%.1f] Temp:%.1f°C Angle:%.1f° Cap:%.1fpF",
               data.distanceUpper1, data.distanceUpper2,
               data.distanceLower1, data.distanceLower2,
               data.temperature, data.angle,
               data.capacitance);
}

bool SensorManager::isDataValid(const SensorData& data) const {
    // 使用SensorData自己的验证
    return data.isAllValid();
}

bool SensorManager::shouldFilterData(const SensorData& newData) const {
    if (!hasData) {
        return false;
    }
    
    // 计算与上次数据的变化百分比
    auto calculateChange = [this](double oldVal, double newVal) -> double {
        if (std::abs(oldVal) < 0.001) {
            return std::abs(newVal) > filterThreshold ? 100.0 : 0.0;
        }
        return std::abs((newVal - oldVal) / oldVal) * 100.0;
    };
    
    // 检查各个传感器的变化
    double changeUpper1 = calculateChange(latestData.distanceUpper1, newData.distanceUpper1);
    double changeUpper2 = calculateChange(latestData.distanceUpper2, newData.distanceUpper2);
    
    // 如果任何传感器变化超过阈值，过滤数据
    return changeUpper1 > filterThreshold || changeUpper2 > filterThreshold;
}

void SensorManager::updateStatistics(bool success, int64_t readTime) {
    std::lock_guard<std::mutex> lock(mutex);
    
    statistics.totalReads++;
    statistics.totalReadTime += readTime;
    statistics.lastReadTime = getCurrentTimestamp();
    
    if (success) {
        statistics.successfulReads++;
    } else {
        statistics.failedReads++;
    }
}

void SensorManager::notifyDataReceived(const SensorData& data) {
    if (dataCallback) {
        // 在锁外调用回调，避免死锁
        std::thread([this, data]() {
            dataCallback(data);
        }).detach();
    }
}

void SensorManager::notifyError(const std::string& error) {
    LOG_ERROR("SensorManager error: " + error);
    
    if (errorCallback) {
        // 在锁外调用回调
        std::thread([this, error]() {
            errorCallback(error);
        }).detach();
    }
}

int64_t SensorManager::getCurrentTimestamp() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<milliseconds>(duration).count();
}