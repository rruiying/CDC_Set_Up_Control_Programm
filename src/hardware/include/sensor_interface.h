#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <chrono>
#include "../../models/include/sensor_data.h"

class SerialInterface;

class SensorInterface {
public:
    explicit SensorInterface(std::shared_ptr<SerialInterface> serial);
    ~SensorInterface();
    
    // 回调函数类型
    using DataCallback = std::function<void(const SensorData&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    // 数据请求
    bool requestAllSensorData();
    bool requestDistanceSensors();
    bool requestAngleSensor();
    bool requestTemperature();
    bool requestCapacitance();
    
    // 自动轮询
    void startPolling(int intervalMs = 100);
    void stopPolling();
    bool isPolling() const { return polling; }
    
    // 数据处理
    void processData(const std::string& data);
    
    // 获取最新数据
    SensorData getLatestData() const;
    
    // 设置回调
    void setDataCallback(DataCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
private:
    void pollThread();
    bool validateSensorData(const SensorData& data);
    
    std::shared_ptr<SerialInterface> m_serial;
    std::unique_ptr<std::thread> m_pollThread;
    std::atomic<bool> polling{false};
    std::atomic<bool> stopPollingFlag{false};
    std::atomic<int> pollInterval{100};
    
    mutable std::mutex dataMutex;
    SensorData m_latestData;
    
    DataCallback dataCallback;
    ErrorCallback errorCallback;
};