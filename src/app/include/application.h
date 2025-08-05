#pragma once
#include <QObject>
#include <memory>

// 前向声明
class SerialInterface;
class MotorInterface;
class SensorInterface;
class MotorController;
class SensorManager;
class SafetyManager;
class CalibrationEngine;
class DataRecorder;
class DataProcessor;
class ExportManager;
class FileManager;
class MainWindow;

class Application : public QObject {
    Q_OBJECT
    
public:
    explicit Application(QObject* parent = nullptr);
    ~Application();
    
    // 初始化
    bool initialize();
    void shutdown();
    
    // 获取组件
    SerialInterface* getSerialInterface() const { return m_serialInterface.get(); }
    MotorController* getMotorController() const { return m_motorController.get(); }
    SensorManager* getSensorManager() const { return m_sensorManager.get(); }
    SafetyManager* getSafetyManager() const { return m_safetyManager.get(); }
    DataRecorder* getDataRecorder() const { return m_dataRecorder.get(); }
    
    // 显示主窗口
    void showMainWindow();
    
    // 应用程序状态
    bool isRunning() const { return m_isRunning; }
    QString getVersion() const { return "1.0.0"; }
    
signals:
    void initialized();
    void shutdownRequested();
    void error(const QString& message);
    
private slots:
    void onEmergencyStop();
    void onCriticalError(const QString& error);
    
private:
    // 硬件层 - 使用 shared_ptr 因为多个组件可能共享
    std::shared_ptr<SerialInterface> m_serialInterface;
    std::unique_ptr<MotorInterface> m_motorInterface;
    std::unique_ptr<SensorInterface> m_sensorInterface;
    
    // 核心层
    std::unique_ptr<MotorController> m_motorController;
    std::unique_ptr<SensorManager> m_sensorManager;
    std::shared_ptr<SafetyManager> m_safetyManager;
    std::unique_ptr<DataRecorder> m_dataRecorder;
    
    // 数据层
    std::unique_ptr<DataProcessor> m_dataProcessor;
    std::unique_ptr<ExportManager> m_exportManager;
    std::unique_ptr<FileManager> m_fileManager;
    
    // UI
    std::unique_ptr<MainWindow> m_mainWindow;
    
    bool m_isRunning;
    
    // 内部方法
    bool initializeHardware();
    bool initializeCore();
    bool initializeData();
    bool initializeUI();
    void connectSignals();
};