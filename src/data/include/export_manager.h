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

/**
 * @brief 导出格式枚举
 */
enum class ExportFormat {
    CSV,
    EXCEL,
    JSON,
    XML,
    TEXT,
    MATLAB,
    CUSTOM
};

/**
 * @brief 导出选项结构
 */
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

/**
 * @brief 导出模板
 */
struct ExportTemplate {
    std::string name;
    std::string description;
    ExportOptions options;
    std::vector<std::string> customFields;
};

/**
 * @brief 导出统计信息
 */
struct ExportStatistics {
    int totalRecords = 0;
    int exportedRecords = 0;
    int64_t exportDuration = 0; // 毫秒
    size_t fileSize = 0;
    std::string filename;
};

/**
 * @brief 导出管理器类
 * 
 * 提供高级的数据导出功能：
 * - 多种格式支持
 * - 灵活的字段选择
 * - 批量导出
 * - 模板管理
 * - 进度回调
 * - 错误处理
 */
class ExportManager {
public:
    /**
     * @brief 构造函数
     */
    ExportManager();
    
    /**
     * @brief 析构函数
     */
    ~ExportManager();
    
    // 回调函数类型
    using ProgressCallback = std::function<void(int percentage)>;
    using CompletionCallback = std::function<void(const ExportStatistics& stats)>;
    
    // 基本导出方法
    bool exportData(const std::vector<MeasurementData>& data,
                   const std::string& filename,
                   const ExportOptions& options = ExportOptions());
    
    // 过滤导出
    bool exportFiltered(const std::vector<MeasurementData>& data,
                       const std::string& filename,
                       const ExportOptions& options,
                       std::function<bool(const MeasurementData&)> filter);
    
    // 批量导出
    bool batchExport(const std::vector<MeasurementData>& data,
                    const std::vector<std::string>& filenames,
                    const std::vector<ExportOptions>& options);
    
    // 模板管理
    void addTemplate(const ExportTemplate& template_);
    void removeTemplate(const std::string& name);
    std::vector<ExportTemplate> getTemplates() const;
    bool exportUsingTemplate(const std::vector<MeasurementData>& data,
                           const std::string& filename,
                           const std::string& templateName);
    
    // 统计导出
    bool exportStatistics(const std::vector<MeasurementData>& data,
                         const std::string& filename);
    
    // 自定义导出
    bool exportCustom(const std::vector<MeasurementData>& data,
                     const std::string& filename,
                     std::function<std::string(const MeasurementData&)> formatter);
    
    // 文件名生成
    std::string generateFilename(const ExportOptions& options) const;
    
    // 格式支持查询
    bool isFormatSupported(ExportFormat format) const;
    std::vector<std::string> getSupportedExtensions() const;
    
    // 回调设置
    void setProgressCallback(ProgressCallback callback);
    void setCompletionCallback(CompletionCallback callback);
    
    // 错误处理
    std::string getLastError() const;
    void clearError();
    
    // 导出统计
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