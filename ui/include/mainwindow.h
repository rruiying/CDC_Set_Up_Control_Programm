#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include <memory>
#include <vector>
#include <string>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QListWidgetItem;
class QResizeEvent;
QT_END_NAMESPACE

class ApplicationController;
class DeviceInfo;  // 前向声明

// 简单的日志级别枚举（避免依赖Logger.h）
enum class LogLevel {
    ALL = 0,
    INFO,
    WARNING,
    ERROR
};

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(ApplicationController* controller, QWidget *parent = nullptr);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
private slots:
    // 设备管理槽函数
    void onAddDeviceClicked();
    void onRemoveDeviceClicked();
    void onConnectDeviceClicked();
    void onDisconnectDeviceClicked();
    void onSendCommandClicked();
    void onDeviceSelectionChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void onCommandLineReturnPressed();
    
    // 电机控制槽函数
    void onSetHeightClicked();
    void onSetAngleClicked();
    void onMoveToPositionClicked();
    void onHomePositionClicked();
    void onStopMotorClicked();
    void onEmergencyStopClicked();
    void onSafetyLimitsChanged();
    void updateDataTable();
    
    // 传感器监控槽函数
    void onRecordDataClicked();
    void onExportDataClicked();
    void onSensorUpdateTimer();
    
    void onLogLevelChanged(int index);
    void onClearLogClicked();
    void onSaveLogClicked();
    void onLogUpdateTimer();
    
    void updateLogDisplay();
    void forceLogUpdate();
    
private:
    void showStatusMessage(const QString& message, int timeout = 3000);
    void initializeDeviceManagement();
    void initializeMotorControl();
    void initializeSensorMonitor();
    void initializeLogViewer();
    void setupCallbacks();
    
    void updateDeviceListDisplay();
    void updateSelectedDeviceDisplay();
    void updateDeviceButtons();
    void updateMotorControlDisplay();
    void updateMotorControlButtons();
    void updateSensorMonitorDisplay();
    void clearCommunicationLog();
    void setupRawSerialCommunication();
    
    void addCommunicationLog(const QString& message, bool isOutgoing);
    void logUserOperation(const QString& operation);
    
    // 设备管理辅助方法
    QStringList getExistingDeviceNames() const;
    QString getDeviceDisplayText(int index) const;
    bool isPortInUse(const QString& port) const;
    void sendCommandToCurrentDevice(const QString& command);
    
    // 电机控制辅助方法
    bool checkSafetyLimits(double height, double angle);
    void updateTargetPositionRanges();
    void updateTheoreticalCapacitance();
    
    // 传感器辅助方法
    double calculateTheoreticalCapacitance() const;
    
    // 日志辅助方法
    QString formatLogEntry(int level, const QString& message) const;
    QString generateLogFilename() const;
    bool saveLogsToFile(const QString& filename) const;
    
    // 回调处理
    void handleConnectionChanged(bool connected, const QString& device);
    void handleDataReceived(const QString& data);
    void handleSensorData(const std::string& jsonData);
    void handleMotorStatusChanged(int status);
    void handleError(const QString& error);
    
private:
    Ui::MainWindow *ui;
    ApplicationController* m_controller;  // 不拥有，只引用
    
    // UI状态变量
    int currentSelectedDeviceIndex = -1;
    LogLevel currentLogLevel = LogLevel::ALL;
    bool isRecording = false;
    bool isInitialized;
    
    // 当前值（从controller获取的缓存）
    double currentHeight = 0.0;
    double currentAngle = 0.0;
    double targetHeight = 0.0;
    double targetAngle = 0.0;
    double theoreticalCapacitance = 0.0;
    
    // 定时器
    QTimer* logUpdateTimer = nullptr;
    QTimer* sensorUpdateTimer = nullptr;
    
    // 时间记录
    QDateTime lastRecordTime;
    
    // 日志显示
    int lastDisplayedLogCount = 0;
    
    // 常量
    static const int MAX_LOG_LINES = 10000;
    static const int MAX_COMM_LOG_LINES = 1000;
    static const int MAX_LOG_DISPLAY_LINES = 5000;
    static const int MAX_COMMUNICATION_LOG_LINES = 1000;
};

#endif // MAINWINDOW_H