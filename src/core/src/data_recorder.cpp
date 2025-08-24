#include "../include/data_recorder.h"
#include <mutex>
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
        if (m_autoSaveThread && m_autoSaveThread->joinable()) {
            m_autoSaveThread->join();
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
    std::lock_guard<std::mutex> lock(mutex);
    maxRecords = max;
    enforceMaxRecords();
}

void DataRecorder::setMemoryLimit(size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex);
    memoryLimit = bytes;
    enforceMemoryLimit();
}

size_t DataRecorder::getEstimatedMemoryUsage() const {
    return estimatedMemoryUsage.load();
}

bool DataRecorder::exportToCSV(const std::string& filename) const {
    std::vector<MeasurementData> dataCopy;
    {
        std::lock_guard<std::mutex> lock(mutex);
        dataCopy.assign(measurements.begin(), measurements.end());
    }
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for export: " + filename);
        return false;
    }
    
    file << MeasurementData::getCSVHeader() << "\n";
    int total = static_cast<int>(dataCopy.size());
    for (int i = 0; i < total; ++i) {
        file << dataCopy[i].toCSV() << "\n";
        notifyExportProgress(i+1, total);
    }
    
    file.close();
    LOG_INFO_F("Exported %d measurements to %s", total, filename.c_str());
    return true;
}

bool DataRecorder::importFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for import: " + filename);
        return false;
    }
    
    clear();
    
    std::string line;
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
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
        
        m_autoSaveThread = std::make_unique<std::thread>(&DataRecorder::autoSaveThreadFunction, this);
        LOG_INFO("Auto-save enabled: " + autoSaveFilename);
        
    } else if (!enable && autoSaveEnabled) {
        autoSaveEnabled = false;
        stopAutoSave = true;
        
        if (m_autoSaveThread && m_autoSaveThread->joinable()) {
            m_autoSaveThread->join();
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
    
    stats.dataCount = measurements.size();
    stats.firstRecordTime = measurements.front().getTimestamp();
    stats.lastRecordTime = measurements.back().getTimestamp();

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
    
    stats.meanHeight = sumHeight / stats.dataCount; 
    stats.meanAngle = sumAngle / stats.dataCount; 
    stats.meanCapacitance = sumCapacitance / stats.dataCount;
    cachedStatistics = stats;
    statisticsValid = true;
    
    return stats;
}

void DataRecorder::setDataChangeCallback(DataChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    dataChangeCallback = std::move(callback);
}

void DataRecorder::setExportProgressCallback(ExportProgressCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    exportProgressCallback = std::move(callback);
}

std::string DataRecorder::getDefaultFilename() const {
    return generateTimestampFilename("measurement_data");
}

std::string DataRecorder::generateTimestampFilename(const std::string& prefix) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    std::stringstream ss;
    ss << prefix << "_";
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    ss << ".csv";
    
    return ss.str();
}

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

void DataRecorder::autoSaveThreadFunction() {
    LOG_INFO("Auto-save thread started");
    
    while (!stopAutoSave) {
        std::this_thread::sleep_for(std::chrono::milliseconds(autoSaveInterval.load()));
        
        if (stopAutoSave) {
            break;
        }
        
        if (hasData()) {
            exportToCSV(autoSaveFilename);
            lastAutoSave = std::chrono::system_clock::now();
        }
    }
    
    LOG_INFO("Auto-save thread stopped");
}

void DataRecorder::notifyDataChange() {
    DataChangeCallback cb;
    {
        std::lock_guard<std::mutex> lock(mutex);
        cb = dataChangeCallback;
    }
    if (cb) {
        int count = getRecordCount();
        cb(count);
    }
}

void DataRecorder::notifyExportProgress(int current, int total) const {
    ExportProgressCallback cb;
    {
        std::lock_guard<std::mutex> lock(mutex);
        cb = exportProgressCallback;
    }
    if (cb) {
        cb(current, total);
    }
}

bool DataRecorder::shouldCompressRecord(const MeasurementData& existing, 
                                       const MeasurementData& newData) const {
    double heightDiff = std::abs(existing.getSetHeight() - newData.getSetHeight());
    double angleDiff = std::abs(existing.getSetAngle() - newData.getSetAngle());
    
    return heightDiff < compressionThreshold && angleDiff < compressionThreshold;
}

size_t DataRecorder::estimateRecordSize(const MeasurementData& record) const {
    return sizeof(MeasurementData) + sizeof(SensorData) + 100;
}