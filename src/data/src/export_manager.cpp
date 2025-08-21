#include "../include/export_manager.h"
#include "../../utils/include/logger.h"
#include "../include/file_manager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <ctime>

ExportManager::ExportManager() {
    // 初始化写缓冲区
    writeBuffer = std::make_unique<char[]>(writeBufferSize);
    
    ExportTemplate csvTemplate;
    csvTemplate.name = "Standard CSV";
    csvTemplate.description = "Standard CSV format with all fields";
    csvTemplate.options.format = ExportFormat::CSV;
    addTemplate(csvTemplate);
    
    LOG_INFO("ExportManager initialized");
}

ExportManager::~ExportManager() = default;

bool ExportManager::exportData(const std::vector<MeasurementData>& data,
                              const std::string& filename,
                              const ExportOptions& options) {
    if (data.empty()) {
        setError("No data to export");
        return false;
    }
    
    auto startTime = std::chrono::steady_clock::now();
    lastExportStats = ExportStatistics();
    lastExportStats.totalRecords = data.size();
    lastExportStats.filename = filename;
    
    bool success = false;
    
    try {
        switch (options.format) {
            case ExportFormat::CSV:
                success = exportCSV(data, filename, options);
                break;
                
            case ExportFormat::JSON:
                success = exportJSON(data, filename, options);
                break;
                
            case ExportFormat::XML:
                success = exportXML(data, filename, options);
                break;
                
            case ExportFormat::TEXT:
                success = exportText(data, filename, options);
                break;
                
            case ExportFormat::MATLAB:
                success = exportMATLAB(data, filename, options);
                break;
                
            case ExportFormat::EXCEL:
                // Excel格式可能需要外部库，这里使用CSV作为替代
                LOG_WARNING("Excel format not fully supported, using CSV instead");
                success = exportCSV(data, filename, options);
                break;
                
            default:
                setError("Unsupported export format");
                return false;
        }
        
        if (success && options.compress) {
            success = compressFile(filename);
        }
        
        if (success) {
            lastExportStats.exportedRecords = data.size();
            FileManager fm;
            lastExportStats.fileSize = fm.getFileSize(filename);
            lastExportStats.exportDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            
            notifyCompletion(lastExportStats);
            LOG_INFO_F("Exported %zu records to %s", data.size(), filename.c_str());
        }
        
    } catch (const std::exception& e) {
        setError(std::string("Export failed: ") + e.what());
        return false;
    }
    
    return success;
}

bool ExportManager::exportFiltered(const std::vector<MeasurementData>& data,
                                  const std::string& filename,
                                  const ExportOptions& options,
                                  std::function<bool(const MeasurementData&)> filter) {
    // 过滤数据
    std::vector<MeasurementData> filteredData;
    std::copy_if(data.begin(), data.end(), std::back_inserter(filteredData), filter);
    
    LOG_INFO_F("Filtered %zu records from %zu total", filteredData.size(), data.size());
    
    return exportData(filteredData, filename, options);
}

bool ExportManager::batchExport(const std::vector<MeasurementData>& data,
                               const std::vector<std::string>& filenames,
                               const std::vector<ExportOptions>& options) {
    if (filenames.size() != options.size()) {
        setError("Filename and options count mismatch");
        return false;
    }
    
    bool allSuccess = true;
    
    for (size_t i = 0; i < filenames.size(); ++i) {
        if (!exportData(data, filenames[i], options[i])) {
            LOG_ERROR_F("Batch export failed for file: %s", filenames[i].c_str());
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

void ExportManager::addTemplate(const ExportTemplate& template_) {
    std::lock_guard<std::mutex> lock(mutex);
    templates[template_.name] = template_;
    LOG_INFO_F("Export template added: %s", template_.name.c_str());
}

void ExportManager::removeTemplate(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    templates.erase(name);
}

std::vector<ExportTemplate> ExportManager::getTemplates() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<ExportTemplate> result;
    
    for (const auto& pair : templates) {
        result.push_back(pair.second);
    }
    
    return result;
}

bool ExportManager::exportUsingTemplate(const std::vector<MeasurementData>& data,
                                       const std::string& filename,
                                       const std::string& templateName) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = templates.find(templateName);
    if (it == templates.end()) {
        setError("Template not found: " + templateName);
        return false;
    }
    
    return exportData(data, filename, it->second.options);
}

bool ExportManager::exportStatistics(const std::vector<MeasurementData>& data,
                                    const std::string& filename) {
    if (data.empty()) {
        setError("No data for statistics");
        return false;
    }
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        setError("Failed to open file: " + filename);
        return false;
    }
    
    // 计算统计信息
    double sumHeight = 0.0, sumAngle = 0.0, sumCap = 0.0;
    double minHeight = data[0].getSetHeight();
    double maxHeight = minHeight;
    double minAngle = data[0].getSetAngle();
    double maxAngle = minAngle;
    
    for (const auto& m : data) {
        double h = m.getSetHeight();
        double a = m.getSetAngle();
        double c = m.getTheoreticalCapacitance();
        
        sumHeight += h;
        sumAngle += a;
        sumCap += c;
        
        minHeight = std::min(minHeight, h);
        maxHeight = std::max(maxHeight, h);
        minAngle = std::min(minAngle, a);
        maxAngle = std::max(maxAngle, a);
    }
    
    double avgHeight = sumHeight / data.size();
    double avgAngle = sumAngle / data.size();
    double avgCap = sumCap / data.size();
    
    // 写入统计信息
    file << "Measurement Data Statistics\n";
    file << "==========================\n\n";
    file << "Total Records: " << data.size() << "\n\n";
    
    file << "Height Statistics:\n";
    file << "  Average Height: " << std::fixed << std::setprecision(2) << avgHeight << " mm\n";
    file << "  Min Height: " << minHeight << " mm\n";
    file << "  Max Height: " << maxHeight << " mm\n";
    file << "  Range: " << (maxHeight - minHeight) << " mm\n\n";
    
    file << "Angle Statistics:\n";
    file << "  Average Angle: " << avgAngle << "°\n";
    file << "  Min Angle: " << minAngle << "°\n";
    file << "  Max Angle: " << maxAngle << "°\n";
    file << "  Range: " << (maxAngle - minAngle) << "°\n\n";
    
    file << "Capacitance Statistics:\n";
    file << "  Average Capacitance: " << avgCap << " pF\n";
    
    // 时间范围
    if (!data.empty()) {
        auto firstTime = formatTimestamp(data.front().getTimestamp(), "%Y-%m-%d %H:%M:%S");
        auto lastTime = formatTimestamp(data.back().getTimestamp(), "%Y-%m-%d %H:%M:%S");
        
        file << "\nTime Range:\n";
        file << "  First Record: " << firstTime << "\n";
        file << "  Last Record: " << lastTime << "\n";
    }
    
    file.close();
    LOG_INFO_F("Statistics exported to %s", filename.c_str());
    return true;
}

bool ExportManager::exportCustom(const std::vector<MeasurementData>& data,
                                const std::string& filename,
                                std::function<std::string(const MeasurementData&)> formatter) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        setError("Failed to open file: " + filename);
        return false;
    }
    
    int current = 0;
    for (const auto& measurement : data) {
        file << formatter(measurement) << "\n";
        current++;
        notifyProgress(current, data.size());
    }
    
    file.close();
    return true;
}

std::string ExportManager::generateFilename(const ExportOptions& options) const {
    std::ostringstream oss;
    
    if (!options.filenamePrefix.empty()) {
        oss << options.filenamePrefix;
        if (options.appendTimestamp) {
            oss << "_";
        }
    }
    
    if (options.appendTimestamp) {
        auto now = std::chrono::system_clock::now();
        auto tt  = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
    #ifdef _WIN32
        localtime_s(&tm, &tt);
    #else
        localtime_r(&tt, &tm);
    #endif
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    }
    
    // 添加扩展名
    switch (options.format) {
        case ExportFormat::CSV:
            oss << ".csv";
            break;
        case ExportFormat::JSON:
            oss << ".json";
            break;
        case ExportFormat::XML:
            oss << ".xml";
            break;
        case ExportFormat::TEXT:
            oss << ".txt";
            break;
        case ExportFormat::MATLAB:
            oss << ".mat";
            break;
        case ExportFormat::EXCEL:
            oss << ".xlsx";
            break;
        default:
            oss << ".dat";
    }
    
    if (options.compress) {
        oss << ".gz";
    }
    
    return oss.str();
}

bool ExportManager::isFormatSupported(ExportFormat format) const {
    switch (format) {
        case ExportFormat::CSV:
        case ExportFormat::JSON:
        case ExportFormat::XML:
        case ExportFormat::TEXT:
            return true;
        case ExportFormat::MATLAB:
        case ExportFormat::EXCEL:
            return false; // 需要外部库支持
        default:
            return false;
    }
}

std::vector<std::string> ExportManager::getSupportedExtensions() const {
    return {".csv", ".json", ".xml", ".txt"};
}

void ExportManager::setProgressCallback(ProgressCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    progressCallback = std::move(callback);
}

void ExportManager::setCompletionCallback(CompletionCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    completionCallback = std::move(callback);
}

std::string ExportManager::getLastError() const {
    std::lock_guard<std::mutex> lock(mutex);
    return lastError;
}

void ExportManager::clearError() {
    std::lock_guard<std::mutex> lock(mutex);
    lastError.clear();
}

ExportStatistics ExportManager::getLastExportStatistics() const {
    return lastExportStats;
}

void ExportManager::setDefaultOptions(const ExportOptions& options) {
    defaultOptions = options;
}

ExportOptions ExportManager::getDefaultOptions() const {
    return defaultOptions;
}

// 私有方法实现

bool ExportManager::exportCSV(const std::vector<MeasurementData>& data,
                             const std::string& filename,
                             const ExportOptions& options) {
    if (!writeBuffer) {
        writeBuffer = std::make_unique<char[]>(writeBufferSize);
    }
    std::ofstream file;
    
    if (options.useBuffering) {
        file.rdbuf()->pubsetbuf(writeBuffer.get(), writeBufferSize);
    }
    
    file.open(filename);
    if (!file.is_open()) {
        setError("Failed to open file: " + filename);
        return false;
    }
    
    // 写入头部
    if (options.includeHeader) {
        file << generateCSVHeader(options) << options.lineEnding;
    }
    
    // 写入数据
    int current = 0;
    for (const auto& measurement : data) {
        file << measurementToCSV(measurement, options) << options.lineEnding;
        current++;
        notifyProgress(current, data.size());
    }
    
    file.close();
    return true;
}

bool ExportManager::exportJSON(const std::vector<MeasurementData>& data,
                              const std::string& filename,
                              const ExportOptions& options) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        setError("Failed to open file: " + filename);
        return false;
    }
    
    file << "[";
    if (options.prettyPrint) file << "\n";
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (options.prettyPrint) file << "  ";
        file << measurementToJSON(data[i], options);
        
        if (i < data.size() - 1) {
            file << ",";
        }
        
        if (options.prettyPrint) file << "\n";
        notifyProgress(i + 1, data.size());
    }
    
    file << "]";
    file.close();
    return true;
}

bool ExportManager::exportXML(const std::vector<MeasurementData>& data,
                             const std::string& filename,
                             const ExportOptions& options) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        setError("Failed to open file: " + filename);
        return false;
    }
    
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<measurements>\n";
    
    for (size_t i = 0; i < data.size(); ++i) {
        file << "  <measurement>\n";
        
        if (options.includeTimestamp) {
            file << "    <timestamp>" << data[i].getTimestamp() << "</timestamp>\n";
        }
        
        if (options.includeSetValues) {
            file << "    <set_height>" << formatValue(data[i].getSetHeight(), options.decimalPlaces) << "</set_height>\n";
            file << "    <set_angle>" << formatValue(data[i].getSetAngle(), options.decimalPlaces) << "</set_angle>\n";
        }
        
        // 添加其他字段...
        
        file << "  </measurement>\n";
        notifyProgress(i + 1, data.size());
    }
    
    file << "</measurements>\n";
    file.close();
    return true;
}

bool ExportManager::exportText(const std::vector<MeasurementData>& data,
                              const std::string& filename,
                              const ExportOptions& options) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        setError("Failed to open file: " + filename);
        return false;
    }
    
    file << "Measurement Data Export\n";
    file << "======================\n\n";
    
    for (size_t i = 0; i < data.size(); ++i) {
        file << "Record " << (i + 1) << ":\n";
        file << data[i].toLogString() << "\n\n";
        notifyProgress(i + 1, data.size());
    }
    
    file.close();
    return true;
}

bool ExportManager::exportMATLAB(const std::vector<MeasurementData>& data,
                                const std::string& filename,
                                const ExportOptions& options) {
    // MATLAB格式需要专门的库支持
    setError("MATLAB format not implemented");
    return false;
}

std::string ExportManager::formatValue(double value, int decimalPlaces) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimalPlaces) << value;
    return oss.str();
}

std::string ExportManager::formatTimestamp(int64_t timestamp, const std::string& format) const {
    auto seconds = timestamp / 1000;
    std::time_t time = static_cast<std::time_t>(seconds);
    std::tm tm{};
    #ifdef _WIN32
    localtime_s(&tm, &time);
    #else
    localtime_r(&time, &tm);
    #endif

    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());
    return oss.str();
}

std::string ExportManager::escapeCSV(const std::string& value, const ExportOptions& options) const {
    if (value.find(options.delimiter) != std::string::npos ||
        value.find(options.quoteChar) != std::string::npos ||
        value.find('\n') != std::string::npos ||
        value.find('\r') != std::string::npos) {
        
        std::string escaped = value;
        // 替换引号
        size_t pos = 0;
        while ((pos = escaped.find(options.quoteChar, pos)) != std::string::npos) {
            escaped.insert(pos, 1, options.quoteChar);
            pos += 2;
        }
        
        return std::string(1, options.quoteChar) + escaped + options.quoteChar;
    }
    
    return value;
}

std::string ExportManager::generateCSVHeader(const ExportOptions& options) const {
    std::vector<std::string> headers;
    
    if (options.includeTimestamp) {
        headers.push_back("Timestamp");
    }
    
    if (options.includeSetValues) {
        headers.push_back("Set_Height(mm)");
        headers.push_back("Set_Angle(deg)");
        headers.push_back("Theoretical_Capacitance(pF)");
    }
    
    if (options.includeSensorData) {
        headers.push_back("Upper_Sensor_1(mm)");
        headers.push_back("Upper_Sensor_2(mm)");
        headers.push_back("Lower_Sensor_1(mm)");
        headers.push_back("Lower_Sensor_2(mm)");
        headers.push_back("Temperature(C)");
        headers.push_back("Measured_Angle(deg)");
        headers.push_back("Measured_Capacitance(pF)");
    }
    
    if (options.includeCalculatedValues) {
        headers.push_back("Average_Height(mm)");
        headers.push_back("Calculated_Angle(deg)");
        headers.push_back("Capacitance_Difference(pF)");
    }
    
    // 组合头部
    std::string result;
    for (size_t i = 0; i < headers.size(); ++i) {
        result += headers[i];
        if (i < headers.size() - 1) {
            result += options.delimiter;
        }
    }
    
    return result;
}

std::string ExportManager::measurementToCSV(const MeasurementData& data, const ExportOptions& options) const {
    std::vector<std::string> values;
    
    if (options.includeTimestamp) {
        values.push_back(formatTimestamp(data.getTimestamp(), options.dateFormat));
    }
    
    if (options.includeSetValues) {
        values.push_back(formatValue(data.getSetHeight(), options.decimalPlaces));
        values.push_back(formatValue(data.getSetAngle(), options.decimalPlaces));
        values.push_back(formatValue(data.getTheoreticalCapacitance(), options.decimalPlaces));
    }
    
    if (options.includeSensorData) {
        const auto& sensor = data.getSensorData();
        // 直接访问公共成员变量，而不是使用不存在的getter方法
        values.push_back(formatValue(sensor.distanceUpper1, options.decimalPlaces));
        values.push_back(formatValue(sensor.distanceUpper2, options.decimalPlaces));
        values.push_back(formatValue(sensor.distanceLower1, options.decimalPlaces));
        values.push_back(formatValue(sensor.distanceLower2, options.decimalPlaces));
        values.push_back(formatValue(sensor.temperature, options.decimalPlaces));
        values.push_back(formatValue(sensor.angle, options.decimalPlaces));
        values.push_back(formatValue(sensor.capacitance, options.decimalPlaces));
    }
    
    if (options.includeCalculatedValues) {
        const auto& sensor = data.getSensorData();
        // 这些方法在SensorData中是存在的
        values.push_back(formatValue(sensor.getAverageHeight(), options.decimalPlaces));
        values.push_back(formatValue(sensor.getCalculatedAngle(), options.decimalPlaces));
        values.push_back(formatValue(data.getCapacitanceDifference(), options.decimalPlaces));
    }
    
    // 组合值
    std::string result;
    for (size_t i = 0; i < values.size(); ++i) {
        result += escapeCSV(values[i], options);
        if (i < values.size() - 1) {
            result += options.delimiter;
        }
    }
    
    return result;
}

std::string ExportManager::measurementToJSON(const MeasurementData& data, const ExportOptions& options) const {
    std::ostringstream json;
    json << "{";
    
    bool first = true;
    
    if (options.includeTimestamp) {
        json << "\"timestamp\":" << data.getTimestamp();
        first = false;
    }
    
    if (options.includeSetValues) {
        if (!first) json << ",";
        json << "\"set_height\":" << formatValue(data.getSetHeight(), options.decimalPlaces);
        json << ",\"set_angle\":" << formatValue(data.getSetAngle(), options.decimalPlaces);
        json << ",\"theoretical_capacitance\":" << formatValue(data.getTheoreticalCapacitance(), options.decimalPlaces);
        first = false;
    }
    
    // 添加其他字段...
    
    json << "}";
    return json.str();
}

void ExportManager::notifyProgress(int current, int total) {
    if (total <= 0) return;
    ProgressCallback cb;
    { std::lock_guard<std::mutex> lock(mutex); cb = progressCallback; }
    if (cb) cb((current * 100) / total);
}

void ExportManager::notifyCompletion(const ExportStatistics& stats) {
    CompletionCallback cb;
    { std::lock_guard<std::mutex> lock(mutex); cb = completionCallback; }
    if (cb) cb(stats);
}

void ExportManager::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(mutex);
    lastError = error;
    LOG_ERROR(error);
}

bool ExportManager::compressFile(const std::string& filename) {
    // 压缩功能需要zlib或类似库的支持
    LOG_WARNING("File compression not implemented");
    return true;
}
