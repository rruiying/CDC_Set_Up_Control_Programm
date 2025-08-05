// src/core/include/data_recorder.h
#ifndef DATA_RECORDER_H
#define DATA_RECORDER_H

#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <chrono>
#include "../../models/include/measurement_data.h"
#include "../../models/include/data_statistics.h"

/**
 * @brief 数据记录器类
 * 
 * 负责记录和管理测量数据：
 * - 数据存储和检索
 * - CSV导入/导出
 * - 自动保存
 * - 数据过滤和统计
 * - 内存管理
 * - 数据备份和恢复
 */
class DataRecorder {
public:
    /**
     * @brief 构造函数
     */
    DataRecorder();
    
    /**
     * @brief 析构函数
     */
    ~DataRecorder();
    
    // 回调函数类型
    using DataChangeCallback = std::function<void(int recordCount)>;
    using ExportProgressCallback = std::function<void(int current, int total)>;
    
    // 数据记录方法
    void recordMeasurement(const MeasurementData& measurement);
    void recordCurrentState(double setHeight, double setAngle, const SensorData& sensorData);
    
    // 数据访问
    bool hasData() const { return !measurements.empty(); }
    int getRecordCount() const;
    MeasurementData getLatestMeasurement() const;
    std::vector<MeasurementData> getAllMeasurements() const;
    std::vector<MeasurementData> getMeasurementsInTimeRange(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) const;
    
    // 数据过滤
    std::vector<MeasurementData> filterMeasurements(
        std::function<bool(const MeasurementData&)> predicate) const;
    
    // 数据管理
    void clear();
    void setMaxRecords(size_t max);
    size_t getMaxRecords() const { return maxRecords; }
    
    // 内存管理
    void setMemoryLimit(size_t bytes);
    size_t getEstimatedMemoryUsage() const;
    
    // 文件操作
    bool exportToCSV(const std::string& filename) const;
    bool exportToCSV(const std::string& filename, 
                     const std::chrono::system_clock::time_point& start,
                     const std::chrono::system_clock::time_point& end) const;
    bool importFromCSV(const std::string& filename);
    
    // 自动保存
    void setAutoSave(bool enable, const std::string& filename = "");
    bool isAutoSaveEnabled() const { return autoSaveEnabled; }
    void setAutoSaveInterval(int intervalMs);
    int getAutoSaveInterval() const { return autoSaveInterval; }
    
    // 备份和恢复
    bool createBackup(const std::string& filename) const;
    bool restoreFromBackup(const std::string& filename);
    
    // 数据压缩
    void setCompressionEnabled(bool enable) { compressionEnabled = enable; }
    void setCompressionThreshold(double threshold) { compressionThreshold = threshold; }
    void compressData();
    
    // 统计信息
    DataStatistics getStatistics() const;
    
    // 回调设置
    void setDataChangeCallback(DataChangeCallback callback);
    void setExportProgressCallback(ExportProgressCallback callback);
    
    // 实用方法
    std::string getDefaultFilename() const;
    static std::string generateTimestampFilename(const std::string& prefix = "data");

    // 记录控制
    bool isRecording() const { return autoSaveEnabled.load(); }
    void stopRecording() { setAutoSave(false); }
    
    // 添加测量数据（addMeasurement 是 recordMeasurement 的别名）
    void addMeasurement(const MeasurementData& measurement) { 
        recordMeasurement(measurement); 
    }
    
private:
    // 内部方法
    void enforceMaxRecords();
    void enforceMemoryLimit();
    void autoSaveThreadFunction();  // 重命名避免冲突
    void notifyDataChange();
    void notifyExportProgress(int current, int total) const;
    bool shouldCompressRecord(const MeasurementData& existing, const MeasurementData& newData) const;
    size_t estimateRecordSize(const MeasurementData& record) const;
    void updateStatistics(const MeasurementData& measurement);
    
    // 成员变量
    mutable std::mutex mutex;
    std::deque<MeasurementData> measurements;
    
    // 限制参数
    std::atomic<size_t> maxRecords{10000};
    std::atomic<size_t> memoryLimit{100 * 1024 * 1024}; // 100MB默认
    
    // 自动保存
    std::atomic<bool> autoSaveEnabled{false};
    std::atomic<int> autoSaveInterval{300000}; // 5分钟默认
    std::string autoSaveFilename;
    std::unique_ptr<std::thread> m_autoSaveThread;  // 重命名避免冲突
    std::atomic<bool> stopAutoSave{false};
    std::chrono::system_clock::time_point lastAutoSave;
    
    // 数据压缩
    std::atomic<bool> compressionEnabled{false};
    std::atomic<double> compressionThreshold{0.1}; // 默认0.1mm/0.1度
    
    // 回调函数
    DataChangeCallback dataChangeCallback;
    ExportProgressCallback exportProgressCallback;
    
    // 统计缓存
    mutable DataStatistics cachedStatistics;
    mutable bool statisticsValid{false};
    
    // 内存使用估算
    mutable std::atomic<size_t> estimatedMemoryUsage{0};
};

#endif // DATA_RECORDER_H