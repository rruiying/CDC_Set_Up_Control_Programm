#pragma once
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <mutex>

// 前向声明
class MotorController;
class SensorManager;
class DataRecorder;
class SafetyManager;
class SerialInterface;
class ExportManager;
class Logger;
struct SensorData;
struct MeasurementData;
struct ExportOptions;
struct ExportStatistics;

// DeviceInfo的简化版本（避免直接依赖models）
struct DeviceInfoData {
    std::string name;
    std::string portName;
    int baudRate;
    int connectionStatus;  // 0=disconnected, 1=connecting, 2=connected, 3=error
    std::string deviceType;
    int errorCount;
    std::string lastError;
};

class ApplicationController {
public:
    ApplicationController();
    ~ApplicationController();
    
    // 初始化（由Application调用）
    void initialize(
        std::shared_ptr<SerialInterface> serial,
        std::shared_ptr<MotorController> motor,
        std::shared_ptr<SensorManager> sensor,
        std::shared_ptr<SafetyManager> safety,
        std::shared_ptr<DataRecorder> recorder,
        std::shared_ptr<ExportManager> exporter
    );
    
    // ===== 设备管理API =====
    bool addDevice(const std::string& name, const std::string& port, int baudRate);
    bool removeDevice(size_t index);
    bool connectDevice(size_t index);
    bool disconnectDevice(size_t index);
    bool sendCommand(const std::string& command);
    std::vector<DeviceInfoData> getDeviceList() const;
    DeviceInfoData getCurrentDevice() const;
    bool isPortInUse(const std::string& port) const;
    std::vector<std::string> getAvailablePorts() const;

    bool setTargetHeight(double height);
    bool setTargetAngle(double angle);
    bool moveToPosition(double height, double angle);
    bool homeMotor();
    bool stopMotor();
    bool emergencyStop();
    void setSafetyLimits(double minHeight, double maxHeight, 
                         double minAngle, double maxAngle);
    bool checkSafetyLimits(double height, double angle) const;
    double calculateTheoreticalCapacitance(double height, double angle) const;
    
    // 获取当前状态
    double getCurrentHeight() const;
    double getCurrentAngle() const;
    int getMotorStatus() const; // 0=idle, 1=moving, 2=error, 3=homing, 4=calibrating
    double getMaxHeight() const;
    double getMinHeight() const;
    double getMaxAngle() const;
    double getMinAngle() const;
    
    // ===== 传感器监控API =====
    bool startSensorMonitoring();
    bool stopSensorMonitoring();
    bool pauseSensorMonitoring();
    bool resumeSensorMonitoring();
    bool isSensorRunning() const;
    std::vector<MeasurementData> getRecordedData() const; 
    
    // 数据记录
    bool recordCurrentData();
    size_t getRecordCount() const;
    void clearRecords();
    
    // 获取传感器数据（JSON格式）
    std::string getCurrentSensorDataJson() const;
    std::string getAllMeasurementsJson() const;
    
    // ===== 数据导出API =====
    bool exportToCSV(const std::string& filename);
    bool exportWithOptions(const std::string& filename, int format);
    std::string getExportStatisticsJson() const;
    std::string generateDefaultFilename() const;
    
    // ===== 日志API =====
    void logOperation(const std::string& operation);
    void logError(const std::string& error);
    void logWarning(const std::string& warning);
    void logInfo(const std::string& info);
    
    // 获取日志（JSON格式）
    std::string getRecentLogsJson(int count = 100) const;
    std::string getLogsByLevelJson(int level) const;
    void clearLogs();
    bool saveLogsToFile(const std::string& filename);
    
    using ConnectionCallback = std::function<void(bool connected, std::string device)>;
    using DataCallback = std::function<void(std::string data)>;
    using SensorCallback = std::function<void(std::string jsonData)>;
    using MotorCallback = std::function<void(int status)>;
    using ErrorCallback = std::function<void(std::string error)>;
    using ProgressCallback = std::function<void(int percentage)>;
    
    void setConnectionCallback(ConnectionCallback cb);
    void setDataCallback(DataCallback cb);
    void setSensorCallback(SensorCallback cb);
    void setMotorCallback(MotorCallback cb);
    void setErrorCallback(ErrorCallback cb);
    void setProgressCallback(ProgressCallback cb);
    
    // ===== 系统配置 =====
    void updateSensorInterval(int intervalMs);
    void setAutoReconnect(bool enable);
    void setDataCompressionEnabled(bool enable);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;  // PIMPL模式隐藏实现细节
};