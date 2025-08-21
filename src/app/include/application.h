#pragma once
#include <QObject>
#include <memory>

// 前向声明
class ApplicationController;  // 添加这个
class SerialInterface;
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
    
    bool initialize();
    void shutdown();
    
    // 获取组件
    SerialInterface* getSerialInterface() const { return m_serialInterface.get(); }
    MotorController* getMotorController() const { return m_motorController.get(); }
    SensorManager* getSensorManager() const { return m_sensorManager.get(); }
    SafetyManager* getSafetyManager() const { return m_safetyManager.get(); }
    DataRecorder* getDataRecorder() const { return m_dataRecorder.get(); }
    ApplicationController* getController() const { return m_controller.get(); }
    
    void showMainWindow();
    
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
    // 硬件层
    std::shared_ptr<SerialInterface> m_serialInterface;
    std::unique_ptr<SensorInterface> m_sensorInterface;
    
    // 核心层
    std::shared_ptr<MotorController> m_motorController;
    std::shared_ptr<SensorManager> m_sensorManager;
    std::shared_ptr<SafetyManager> m_safetyManager;
    std::shared_ptr<DataRecorder> m_dataRecorder;
    
    // 数据层
    std::unique_ptr<DataProcessor> m_dataProcessor;
    std::shared_ptr<ExportManager> m_exportManager;
    std::unique_ptr<FileManager> m_fileManager;
    
    // UI和控制器
    std::unique_ptr<MainWindow> m_mainWindow;
    std::unique_ptr<ApplicationController> m_controller;
    
    bool m_isRunning;
    
    // 内部方法
    bool initializeHardware();
    bool initializeCore();
    bool initializeData();
    bool initializeUI();
    void connectSignals();
};