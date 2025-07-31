#pragma once
#include <QMainWindow>
#include <QTimer>
#include <memory>
#include "models/include/sensor_data.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class SerialInterface;
class MotorController;
class SensorManager;
class DataRecorder;
class AddDeviceDialog;
class ManualCalibrationDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    MainWindow(SerialInterface* serial = nullptr,
               MotorController* motor = nullptr,
               SensorManager* sensor = nullptr,
               DataRecorder* recorder = nullptr,
               QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    // 串口通信页面
    void onAddDeviceClicked();
    void onDisconnectClicked();
    void onSendCommandClicked();
    void onDeviceSelected();
    void updatePortList();
    
    // 电机控制页面
    void onSetHeightClicked();
    void onSetAngleClicked();
    void onMoveToPositionClicked();
    void onStopMotorClicked();
    void onHomePositionClicked();
    void onEmergencyStopClicked();
    void onAutoCalibateClicked();
    void onManualCalibrateClicked();
    
    // 传感器监控页面
    void onRecordDataClicked();
    void onExportDataClicked();
    void updateSensorDisplay();
    
    // 日志查看页面
    void onClearLogClicked();
    void onBrowseFileClicked();
    void onLoadDataClicked();
    void updateLogFilter();
    
    // 数据可视化页面
    void onChartTypeChanged();
    void onLoadCSVClicked();
    void onStartRealtimeClicked();
    
    // 串口信号
    void onSerialConnected(const QString& port);
    void onSerialDisconnected();
    void onSerialDataReceived(const QString& data);
    void onSerialError(const QString& error);
    
    // 其他信号
    void onMotorStatusChanged();
    void onSensorDataUpdated(const SensorData& data);
    void onLogMessage(const QString& message);
    
private:
    Ui::MainWindow *ui;
    
    // 核心组件引用
    SerialInterface* m_serialInterface;
    MotorController* m_motorController;
    SensorManager* m_sensorManager;
    DataRecorder* m_dataRecorder;
    
    // 对话框
    std::unique_ptr<AddDeviceDialog> m_addDeviceDialog;
    std::unique_ptr<ManualCalibrationDialog> m_calibrationDialog;
    
    // 定时器
    QTimer* m_updateTimer;
    
    // 状态
    SensorData m_lastSensorData;
    bool m_isRecording;
    
    // 初始化方法
    void setupUI();
    void setupConnections();
    void loadConfiguration();
    void updateUIState(bool connected);
    void updateMotorControlUI();
    void updateVisualization();
};