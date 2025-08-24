// src/models/include/system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>

class SystemConfig {
public:
    /**
     * @brief 获取配置单例实例
     * @return SystemConfig实例引用
     */
    static SystemConfig& getInstance();
    
    // 删除拷贝构造和赋值操作
    SystemConfig(const SystemConfig&) = delete;
    SystemConfig& operator=(const SystemConfig&) = delete;
    
    // 文件操作
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    
    // ===== 安全限位 =====
    void setSafetyLimits(double minHeight, double maxHeight, double minAngle, double maxAngle);
    void setHeightLimits(double minHeight, double maxHeight);
    void setAngleLimits(double minAngle, double maxAngle);
    
    double getMinHeight() const { return minHeight; }
    double getMaxHeight() const { return maxHeight; }
    double getMinAngle() const { return minAngle; }
    double getMaxAngle() const { return maxAngle; }
    
    bool isHeightInRange(double height) const;
    bool isAngleInRange(double angle) const;
    bool isPositionValid(double height, double angle) const;
    
    // ===== 电容板参数 =====
    bool setPlateArea(double area);
    bool setDielectricConstant(double epsilon);
    double getPlateArea() const { return plateArea; }
    double getDielectricConstant() const { return dielectricConstant; }
    
    // ===== 系统尺寸 =====
    void setSystemDimensions(double totalHeight, double middlePlateHeight, double sensorSpacing);
    double getTotalHeight() const { return totalHeight; }
    double getMiddlePlateHeight() const { return middlePlateHeight; }
    double getSensorSpacing() const { return sensorSpacing; }
    
    // 新增的系统高度设置方法
    void setSystemHeight(double height) { totalHeight = height; notifyChange(); }
    double getSystemHeight() const { return totalHeight; }
    
    // ===== 电机控制 =====
    void setHomePosition(double height, double angle);
    double getHomeHeight() const { return homeHeight; }
    double getHomeAngle() const { return homeAngle; }
    
    // ===== 通信参数 =====
    void setDefaultBaudRate(int baudRate) { defaultBaudRate = baudRate; notifyChange(); }
    void setCommunicationTimeout(int timeout) { communicationTimeout = timeout; notifyChange(); }
    void setRetryCount(int count) { retryCount = count; notifyChange(); }
    
    int getDefaultBaudRate() const { return defaultBaudRate; }
    int getCommunicationTimeout() const { return communicationTimeout; }
    int getRetryCount() const { return retryCount; }
    
    std::vector<int> getSupportedBaudRates() const;
    
    // ===== 数据记录参数 =====
    void setSensorUpdateInterval(int interval) { sensorUpdateInterval = interval; notifyChange(); }
    void setMaxRecords(int max) { maxRecords = max; notifyChange(); }
    void setAutoSaveInterval(int interval) { autoSaveInterval = interval; notifyChange(); }
    
    int getSensorUpdateInterval() const { return sensorUpdateInterval; }
    int getMaxRecords() const { return maxRecords; }
    int getAutoSaveInterval() const { return autoSaveInterval; }
    
    // ===== 配置变更通知 =====
    using ConfigChangeCallback = std::function<void()>;
    void setConfigChangeCallback(ConfigChangeCallback callback);
    
    // 重置到默认值
    void reset();
    
    // 获取配置摘要
    std::string getConfigSummary() const;
    
    // 友元类声明，允许unique_ptr删除
    friend class std::default_delete<SystemConfig>;
    
private:
    SystemConfig();
    ~SystemConfig() = default;
    
    // 通知配置变更
    void notifyChange();
    
    // 验证方法
    bool validateSafetyLimits(double minH, double maxH, double minA, double maxA) const;
    
    // 成员变量
    mutable std::mutex mutex;
    
    // 安全限位
    double minHeight = 0.0;
    double maxHeight = 150.0;
    double minAngle = -90.0;
    double maxAngle = 90.0;
    
    // 电容板参数
    double plateArea = 2500.0;   // mmm² (50mm x 50mm)
    double dielectricConstant = 1.0;
    
    // 系统尺寸
    double totalHeight = 150.0;      // mm
    double middlePlateHeight = 25.0; // mm
    double sensorSpacing = 80.0;     // mm
    
    double homeHeight = 0.0;         // mm，改为0
    double homeAngle = 0.0;          // degrees
    
    // 通信参数
    int defaultBaudRate = 115200;
    int communicationTimeout = 5000; // ms
    int retryCount = 3;
    
    // 数据记录参数
    int sensorUpdateInterval = 2000; // ms
    int maxRecords = 10000;
    int autoSaveInterval = 300000;   // ms (5 minutes)
    
    // 回调函数
    ConfigChangeCallback changeCallback;
    
    // 单例实例
    static std::unique_ptr<SystemConfig> instance;
    static std::mutex instanceMutex;
};

#endif // SYSTEM_CONFIG_H