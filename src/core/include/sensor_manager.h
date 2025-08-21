#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <deque>
#include <chrono>
#include "../../models/include/sensor_data.h"

// 前向声明
class SerialInterface;

/**
 * @brief 传感器统计信息
 */
struct SensorStatistics {
    int totalReads = 0;
    int successfulReads = 0;
    int failedReads = 0;
    double successRate = 0.0;
    int64_t lastReadTime = 0;
    int64_t totalReadTime = 0;  // 总读取时间（毫秒）
    double averageReadTime = 0.0; // 平均读取时间
};

/**
 * @brief 传感器管理器类
 * 
 * 负责管理传感器数据的定时读取：
 * - 自动定时读取
 * - 数据过滤和验证
 * - 历史数据管理
 * - 统计信息
 * - 回调通知
 */
class SensorManager {
public:
    /**
     * @brief 构造函数
     * @param serialInterface 串口接口（共享指针）
     */
    explicit SensorManager(std::shared_ptr<SerialInterface> serialInterface);
    
    /**
     * @brief 析构函数
     */
    ~SensorManager();
    
    // 回调函数类型
    using DataCallback = std::function<void(const SensorData&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    // 控制方法
    bool start();                  // 启动自动读取
    void stop();                   // 停止自动读取
    void pause();                  // 暂停读取
    void resume();                 // 恢复读取
    bool isRunning() const { return running && !stopRequested; }
    bool isPaused() const { return paused; }
    
    // 手动读取
    bool readSensorsOnce();        // 执行一次读取
    
    // 数据访问
    bool hasValidData() const;
    SensorData getLatestData() const;
    std::vector<SensorData> getDataHistory() const;
    SensorData getAverageData(size_t count) const; // 获取最近n个数据的平均值
    
    // 配置方法
    void setUpdateInterval(int intervalMs);
    int getUpdateInterval() const { return updateInterval; }
    
    void setReadTimeout(int timeoutMs) { readTimeout = timeoutMs; }
    int getReadTimeout() const { return readTimeout; }
    
    void setHistorySize(size_t size);
    size_t getHistorySize() const { return maxHistorySize; }
    
    // 数据过滤
    void setFilteringEnabled(bool enable) { filteringEnabled = enable; }
    bool isFilteringEnabled() const { return filteringEnabled; }
    void setFilterThreshold(double threshold) { filterThreshold = threshold; }
    
    // 回调设置
    void setDataCallback(DataCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    // 统计信息
    SensorStatistics getStatistics() const;
    int getReadCount() const;
    void resetStatistics();
    
    // 重置
    void reset();
    
private:
    // 内部方法
    void updateThread();           // 更新线程函数
    bool performRead();            // 执行读取操作
    void processNewData(const SensorData& data);
    bool isDataValid(const SensorData& data) const;
    bool shouldFilterData(const SensorData& newData) const;
    void updateStatistics(bool success, int64_t readTime);
    void notifyDataReceived(const SensorData& data);
    void notifyError(const std::string& error);
    
    // 成员变量
    std::shared_ptr<SerialInterface> serial;
    
    // 线程控制
    std::unique_ptr<std::thread> updateThreadPtr;
    std::atomic<bool> running{false};
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> paused{false};
    std::condition_variable cv;
    mutable std::mutex mutex;
    
    // 数据存储
    SensorData latestData;
    std::deque<SensorData> dataHistory;
    bool hasData{false};
    
    // 配置参数
    std::atomic<int> updateInterval{2000};  // 默认2秒
    std::atomic<int> readTimeout{1000};     // 默认1秒超时
    size_t maxHistorySize{100};             // 默认保留100条历史
    
    // 数据过滤
    std::atomic<bool> filteringEnabled{false};
    std::atomic<double> filterThreshold{50.0}; // 默认50%变化阈值
    
    // 回调函数
    DataCallback dataCallback;
    ErrorCallback errorCallback;
    
    // 统计信息
    mutable SensorStatistics statistics;
};

#endif // SENSOR_MANAGER_H