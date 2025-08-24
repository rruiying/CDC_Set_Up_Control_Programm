// src/data/include/export_manager.h
#ifndef EXPORT_MANAGER_H
#define EXPORT_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include "../../models/include/measurement_data.h"

enum class ExportFormat {
    CSV,
    EXCEL,
    JSON,
    XML,
    TEXT,
    MATLAB,
    CUSTOM
};

struct ExportOptions {
    ExportFormat format = ExportFormat::CSV;
    
    // CSV选项
    std::string delimiter = ",";
    std::string lineEnding = "\r\n";
    bool includeHeader = true;
    char quoteChar = '"';
    
    // 字段选择
    bool includeTimestamp = true;
    bool includeSetValues = true;
    bool includeSensorData = true;
    bool includeCalculatedValues = true;
    bool includeStatistics = false;
    
    // 格式化选项
    int decimalPlaces = 2;
    std::string dateFormat = "%Y-%m-%d %H:%M:%S";
    bool prettyPrint = false;
    
    // 性能选项
    bool useBuffering = true;
    size_t bufferSize = 1024 * 1024; // 1MB
    bool compress = false;
    
    // 文件名选项
    bool autoGenerateFilename = false;
    std::string filenamePrefix = "data";
    bool appendTimestamp = true;
};

struct ExportTemplate {
    std::string name;
    std::string description;
    ExportOptions options;
    std::vector<std::string> customFields;
};

struct ExportStatistics {
    int totalRecords = 0;
    int exportedRecords = 0;
    int64_t exportDuration = 0;
    size_t fileSize = 0;
    std::string filename;
};

class ExportManager {
public:
    ExportManager();
    ~ExportManager();
    
    using ProgressCallback = std::function<void(int percentage)>;
    using CompletionCallback = std::function<void(const ExportStatistics& stats)>;
    
    bool exportData(const std::vector<MeasurementData>& data,
                   const std::string& filename,
                   const ExportOptions& options = ExportOptions());
    
    bool exportFiltered(const std::vector<MeasurementData>& data,
                       const std::string& filename,
                       const ExportOptions& options,
                       std::function<bool(const MeasurementData&)> filter);
    
    bool batchExport(const std::vector<MeasurementData>& data,
                    const std::vector<std::string>& filenames,
                    const std::vector<ExportOptions>& options);
    
    void addTemplate(const ExportTemplate& template_);
    void removeTemplate(const std::string& name);
    std::vector<ExportTemplate> getTemplates() const;
    bool exportUsingTemplate(const std::vector<MeasurementData>& data,
                           const std::string& filename,
                           const std::string& templateName);
    
    bool exportStatistics(const std::vector<MeasurementData>& data,
                         const std::string& filename);
    
    bool exportCustom(const std::vector<MeasurementData>& data,
                     const std::string& filename,
                     std::function<std::string(const MeasurementData&)> formatter);
    
    std::string generateFilename(const ExportOptions& options) const;
    
    bool isFormatSupported(ExportFormat format) const;
    std::vector<std::string> getSupportedExtensions() const;
    
    void setProgressCallback(ProgressCallback callback);
    void setCompletionCallback(CompletionCallback callback);
    
    std::string getLastError() const;
    void clearError();
    
    ExportStatistics getLastExportStatistics() const;
    
    // 配置
    void setDefaultOptions(const ExportOptions& options);
    ExportOptions getDefaultOptions() const;
    
private:
    // 内部导出方法
    bool exportCSV(const std::vector<MeasurementData>& data,
                  const std::string& filename,
                  const ExportOptions& options);
    
    bool exportJSON(const std::vector<MeasurementData>& data,
                   const std::string& filename,
                   const ExportOptions& options);
    
    bool exportXML(const std::vector<MeasurementData>& data,
                  const std::string& filename,
                  const ExportOptions& options);
    
    bool exportText(const std::vector<MeasurementData>& data,
                   const std::string& filename,
                   const ExportOptions& options);
    
    bool exportMATLAB(const std::vector<MeasurementData>& data,
                     const std::string& filename,
                     const ExportOptions& options);
    
    // 辅助方法
    std::string formatValue(double value, int decimalPlaces) const;
    std::string formatTimestamp(int64_t timestamp, const std::string& format) const;
    std::string escapeCSV(const std::string& value, const ExportOptions& options) const;
    std::string generateCSVHeader(const ExportOptions& options) const;
    std::string measurementToCSV(const MeasurementData& data, const ExportOptions& options) const;
    std::string measurementToJSON(const MeasurementData& data, const ExportOptions& options) const;
    
    void notifyProgress(int current, int total);
    void notifyCompletion(const ExportStatistics& stats);
    void setError(const std::string& error);
    
    bool compressFile(const std::string& filename);
    
    // 成员变量
    mutable std::mutex mutex;
    
    // 模板存储
    std::map<std::string, ExportTemplate> templates;
    
    // 默认选项
    ExportOptions defaultOptions;
    
    // 回调函数
    ProgressCallback progressCallback;
    CompletionCallback completionCallback;
    
    // 错误信息
    mutable std::string lastError;
    
    // 统计信息
    ExportStatistics lastExportStats;
    
    // 缓冲区
    std::unique_ptr<char[]> writeBuffer;
    size_t writeBufferSize = 1024 * 1024; // 1MB
};

#endif // EXPORT_MANAGER_H