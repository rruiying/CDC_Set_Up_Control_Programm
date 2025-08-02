// src/core/src/data_recorder.cpp
#include "../include/data_recorder.h"
#include "../../models/include/system_config.h"
#include "../../utils/include/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>

DataRecorder::DataRecorder() {
    LOG_INFO("DataRecorder initialized");
}

DataRecorder::~DataRecorder() {
    // 停止自动保存线程
    if (autoSaveEnabled) {
        stopAutoSave = true;
        if (autoSaveThread && autoSaveThread->joinable()) {
            autoSaveThread->join();
        }
    }
}

void DataRecorder::recordMeasurement(const MeasurementData& measurement) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        
        // 检查是否需要压缩
        if (compressionEnabled && !measurements.empty()) {
            const auto& lastMeasurement = measurements.back();
            if (shouldCompressRecord(lastMeasurement, measurement)) {
                // 跳过相似的记录
                return;
            }
        }
        
        measurements.push_back(measurement);
        estimatedMemoryUsage += estimateRecordSize(measurement);
        statisticsValid = false;
        
        // 执行限制检查
        enforceMaxRecords();
        enforceMemoryLimit();
    }
    
    notifyDataChange();
    LOG_INFO_F("Measurement recorded: Height=%.1fmm, Angle=%.1f°", 
               measurement.getSetHeight(), measurement.getSetAngle());
}

void DataRecorder::recordCurrentState(double setHeight, double setAngle, const SensorData& sensorData) {
    MeasurementData measurement(setHeight, setAngle, sensorData);
    recordMeasurement(measurement);
}

int DataRecorder::getRecordCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return static_cast<int>(measurements.size());
}

MeasurementData DataRecorder::getLatestMeasurement() const {
    std::lock_guard<std::mutex> lock(mutex);
    if (measurements.empty()) {
        return MeasurementData();
    }
    return measurements.back();
}

std::vector<MeasurementData> DataRecorder::getAllMeasurements() const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::vector<MeasurementData>(measurements.begin(), measurements.end());
}

std::vector<MeasurementData> DataRecorder::getMeasurementsInTimeRange(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end) const {
    
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<MeasurementData> result;
    
    auto startMs = std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count();
    auto endMs = std::chrono::duration_cast<std::chrono::milliseconds>(end.time_since_epoch()).count();
    
    for (const auto& measurement : measurements) {
        int64_t timestamp = measurement.getTimestamp();
        if (timestamp >= startMs && timestamp <= endMs) {
            result.push_back(measurement);
        }
    }
    
    return result;
}

std::vector<MeasurementData> DataRecorder::filterMeasurements(
    std::function<bool(const MeasurementData&)> predicate) const {
    
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<MeasurementData> result;
    
    std::copy_if(measurements.begin(), measurements.end(),
                 std::back_inserter(result), predicate);
    
    return result;
}

void DataRecorder::clear() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        measurements.clear();
        estimatedMemoryUsage = 0;
        statisticsValid = false;
    }
    
    notifyDataChange();
    LOG_INFO("All measurements cleared");
}

void DataRecorder::setMaxRecords(size_t max) {
    maxRecords = max;
    enforceMaxRecords();
}

void DataRecorder::setMemoryLimit(size_t bytes) {
    memoryLimit = bytes;
    enforceMemoryLimit();
}

size_t DataRecorder::getEstimatedMemoryUsage() const {
    return estimatedMemoryUsage.load();
}

bool DataRecorder::exportToCSV(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for export: " + filename);
        return false;
    }
    
    // 写入头部
    file << MeasurementData::getCSVHeader() << "\n";
    
    // 写入数据
    int current = 0;
    for (const auto& measurement : measurements) {
        file << measurement.toCSV() << "\n";
        current++;
        notifyExportProgress(current, measurements.size());
    }
    
    file.close();
    LOG_INFO_F("Exported %zu measurements to %s", measurements.size(), filename.c_str());
    return true;
}

bool DataRecorder::exportToCSV(const std::string& filename,
                               const std::chrono::system_clock::time_point& start,
                               const std::chrono::system_clock::time_point& end) const {
    
    auto filteredData = getMeasurementsInTimeRange(start, end);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for export: " + filename);
        return false;
    }
    
    file << MeasurementData::getCSVHeader() << "\n";
    
    int current = 0;
    for (const auto& measurement : filteredData) {
        file << measurement.toCSV() << "\n";
        current++;
        notifyExportProgress(current, filteredData.size());
    }
    
    file.close();
    LOG_INFO_F("Exported %zu measurements to %s", filteredData.size(), filename.c_str());
    return true;
}

bool DataRecorder::importFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for import: " + filename);
        return false;
    }
    
    // 清空现有数据
    clear();
    
    std::string line;
    // 跳过头部
    std::getline(file, line);
    
    // 读取数据行
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        // 这里需要实现CSV解析逻辑
        // 简化版本：假设数据格式正确
        // 实际应用中应该使用更健壮的CSV解析库
        
        // 创建测试数据（实际应该从CSV解析）
        SensorData sensorData;
        MeasurementData measurement(0.0, 0.0, sensorData);
        
        if (measurement.isValid()) {
            recordMeasurement(measurement);
        }
    }
    
    file.close();
    LOG_INFO_F("Imported measurements from %s", filename.c_str());
    return true;
}

void DataRecorder::setAutoSave(bool enable, const std::string& filename) {
    if (enable && !autoSaveEnabled) {
        autoSaveEnabled = true;
        autoSaveFilename = filename.empty() ? generateTimestampFilename("autosave") : filename;
        stopAutoSave = false;
        
        // 启动自动保存线程
        autoSaveThread = std::make_unique<std::thread>(&DataRecorder::autoSaveThread, this);
        LOG_INFO("Auto-save enabled: " + autoSaveFilename);
        
    } else if (!enable && autoSaveEnabled) {
        autoSaveEnabled = false;
        stopAutoSave = true;
        
        if (autoSaveThread && autoSaveThread->joinable()) {
            autoSaveThread->join();
        }
        LOG_INFO("Auto-save disabled");
    }
}

void DataRecorder::setAutoSaveInterval(int intervalMs) {
    autoSaveInterval = intervalMs;
}

bool DataRecorder::createBackup(const std::string& filename) const {
    return exportToCSV(filename);
}

bool DataRecorder::restoreFromBackup(const std::string& filename) {
    return importFromCSV(filename);
}

void DataRecorder::compressData() {
    if (!compressionEnabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    if (measurements.size() < 2) {
        return;
    }
    
    std::deque<MeasurementData> compressed;
    compressed.push_back(measurements.front());
    
    for (size_t i = 1; i < measurements.size(); ++i) {
        if (!shouldCompressRecord(compressed.back(), measurements[i])) {
            compressed.push_back(measurements[i]);
        }
    }
    
    size_t removed = measurements.size() - compressed.size();
    measurements = std::move(compressed);
    
    LOG_INFO_F("Data compression removed %zu similar records", removed);
}

DataStatistics DataRecorder::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (statisticsValid) {
        return cachedStatistics;
    }
    
    DataStatistics stats;
    
    if (measurements.empty()) {
        return stats;
    }
    
    stats.totalRecords = measurements.size();
    stats.firstRecordTime = measurements.front().getTimestamp();
    stats.lastRecordTime = measurements.back().getTimestamp();
    
    // 初始化最小/最大值
    stats.minHeight = measurements.front().getSetHeight();
    stats.maxHeight = stats.minHeight;
    stats.minAngle = measurements.front().getSetAngle();
    stats.maxAngle = stats.minAngle;
    stats.minCapacitance = measurements.front().getTheoreticalCapacitance();
    stats.maxCapacitance = stats.minCapacitance;
    
    double sumHeight = 0.0;
    double sumAngle = 0.0;
    double sumCapacitance = 0.0;
    
    for (const auto& m : measurements) {
        double height = m.getSetHeight();
        double angle = m.getSetAngle();
        double cap = m.getTheoreticalCapacitance();
        
        sumHeight += height;
        sumAngle += angle;
        sumCapacitance += cap;
        
        stats.minHeight = std::min(stats.minHeight, height);
        stats.maxHeight = std::max(stats.maxHeight, height);
        stats.minAngle = std::min(stats.minAngle, angle);
        stats.maxAngle = std::max(stats.maxAngle, angle);
        stats.minCapacitance = std::min(stats.minCapacitance, cap);
        stats.maxCapacitance = std::max(stats.maxCapacitance, cap);
    }
    
    stats.averageHeight = sumHeight / stats.totalRecords;
    stats.averageAngle = sumAngle / stats.totalRecords;
    stats.averageCapacitance = sumCapacitance / stats.totalRecords;
    
    cachedStatistics = stats;
    statisticsValid = true;
    
    return stats;
}

void DataRecorder::setDataChangeCallback(DataChangeCallback callback) {
    dataChangeCallback = callback;
}

void DataRecorder::setExportProgressCallback(ExportProgressCallback callback) {
    exportProgressCallback = callback;
}

std::string DataRecorder::getDefaultFilename() const {
    return generateTimestampFilename("measurement_data");
}

std::string DataRecorder::generateTimestampFilename(const std::string& prefix) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << prefix << "_";
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << ".csv";
    
    return ss.str();
}

// 私有方法实现

void DataRecorder::enforceMaxRecords() {
    while (measurements.size() > maxRecords) {
        estimatedMemoryUsage -= estimateRecordSize(measurements.front());
        measurements.pop_front();
        statisticsValid = false;
    }
}

void DataRecorder::enforceMemoryLimit() {
    while (estimatedMemoryUsage > memoryLimit && !measurements.empty()) {
        estimatedMemoryUsage -= estimateRecordSize(measurements.front());
        measurements.pop_front();
        statisticsValid = false;
    }
}

void DataRecorder::autoSaveThread() {
    LOG_INFO("Auto-save thread started");
    
    while (!stopAutoSave) {
        std::this_thread::sleep_for(std::chrono::milliseconds(autoSaveInterval.load()));
        
        if (stopAutoSave) {
            break;
        }
        
        // 执行自动保存
        if (hasData()) {
            exportToCSV(autoSaveFilename);
            lastAutoSave = std::chrono::system_clock::now();
        }
    }
    
    LOG_INFO("Auto-save thread stopped");
}

void DataRecorder::notifyDataChange() {
    if (dataChangeCallback) {
        int count = getRecordCount();
        std::thread([this, count]() {
            dataChangeCallback(count);
        }).detach();
    }
}

void DataRecorder::notifyExportProgress(int current, int total) const {
    if (exportProgressCallback) {
        const_cast<DataRecorder*>(this)->exportProgressCallback(current, total);
    }
}

bool DataRecorder::shouldCompressRecord(const MeasurementData& existing, 
                                       const MeasurementData& newData) const {
    double heightDiff = std::abs(existing.getSetHeight() - newData.getSetHeight());
    double angleDiff = std::abs(existing.getSetAngle() - newData.getSetAngle());
    
    return heightDiff < compressionThreshold && angleDiff < compressionThreshold;
}

size_t DataRecorder::estimateRecordSize(const MeasurementData& record) const {
    // 粗略估计每条记录的内存占用
    // MeasurementData基本大小 + SensorData大小 + 字符串等
    return sizeof(MeasurementData) + sizeof(SensorData) + 100;
}