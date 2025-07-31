#include "ui/include/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/include/adddevicedialog.h"
#include "ui/include/manualcalibrationdialog.h"
#include "hardware/include/serial_interface.h"
#include "core/include/motor_controller.h"
#include "core/include/sensor_manager.h"
#include "core/include/data_recorder.h"
#include "utils/include/logger.h"
#include "utils/include/config_manager.h"
#include "data/include/export_manager.h"
#include <QMessageBox>
#include <QFileDialog>

MainWindow::MainWindow(SerialInterface* serial,
                      MotorController* motor,
                      SensorManager* sensor,
                      DataRecorder* recorder,
                      QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serialInterface(serial)
    , m_motorController(motor)
    , m_sensorManager(sensor)
    , m_dataRecorder(recorder)
    , m_updateTimer(new QTimer(this))
    , m_isRecording(false)
{
    ui->setupUi(this);
    
    setupUI();
    setupConnections();
    loadConfiguration();
    
    // 启动更新定时器
    m_updateTimer->start(100); // 100ms更新一次
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupUI() {
    setWindowTitle("CDC Setup Control Program");
    
    // 设置初始页面
    ui->tabWidget->setCurrentIndex(0);
    
    // 设置初始状态
    updateUIState(false);
    
    // 更新端口列表
    updatePortList();
    
    // 初始化速度选择
    ui->comboBox->setCurrentIndex(1); // Medium
    
    // 设置数值范围
    ui->doubleSpinBox->setRange(0, 100);    // 高度
    ui->doubleSpinBox_2->setRange(-45, 45); // 角度
}

void MainWindow::setupConnections() {
    // 串口通信页面
    connect(ui->pushButton_3, &QPushButton::clicked,
            this, &MainWindow::onAddDeviceClicked);
    connect(ui->pushButton, &QPushButton::clicked,
            this, &MainWindow::onDisconnectClicked);
    connect(ui->pushButton_2, &QPushButton::clicked,
            this, &MainWindow::onSendCommandClicked);
    connect(ui->listWidget, &QListWidget::itemClicked,
            this, &MainWindow::onDeviceSelected);
    
    // 电机控制页面
    connect(ui->pushButton_5, &QPushButton::clicked,
            this, &MainWindow::onSetHeightClicked);
    connect(ui->pushButton_9, &QPushButton::clicked,
            this, &MainWindow::onSetAngleClicked);
    connect(ui->pushButton_7, &QPushButton::clicked,
            this, &MainWindow::onMoveToPositionClicked);
    connect(ui->pushButton_6, &QPushButton::clicked,
            this, &MainWindow::onStopMotorClicked);
    connect(ui->pushButton_8, &QPushButton::clicked,
            this, &MainWindow::onHomePositionClicked);
    connect(ui->pushButton_12, &QPushButton::clicked,
            this, &MainWindow::onEmergencyStopClicked);
    connect(ui->pushButton_10, &QPushButton::clicked,
            this, &MainWindow::onAutoCalibateClicked);
    connect(ui->pushButton_11, &QPushButton::clicked,
            this, &MainWindow::onManualCalibrateClicked);
    
    // 传感器监控页面
    connect(ui->pushButton_13, &QPushButton::clicked,
            this, &MainWindow::onRecordDataClicked);
    connect(ui->pushButton_14, &QPushButton::clicked,
            this, &MainWindow::onExportDataClicked);
    
    // 日志查看页面
    connect(ui->pushButton_15, &QPushButton::clicked,
            this, &MainWindow::onClearLogClicked);
    connect(ui->pushButton_16, &QPushButton::clicked,
            this, &MainWindow::onBrowseFileClicked);
    connect(ui->pushButton_17, &QPushButton::clicked,
            this, &MainWindow::onLoadDataClicked);
    connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateLogFilter);
    
    // 数据可视化页面
    connect(ui->comboBox_3, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onChartTypeChanged);
    connect(ui->pushButton_18, &QPushButton::clicked,
            this, &MainWindow::onLoadCSVClicked);
    connect(ui->pushButton_19, &QPushButton::clicked,
            this, &MainWindow::onStartRealtimeClicked);
    
    // 串口信号
    if (m_serialInterface) {
        connect(m_serialInterface, &SerialInterface::connected,
                this, &MainWindow::onSerialConnected);
        connect(m_serialInterface, &SerialInterface::disconnected,
                this, &MainWindow::onSerialDisconnected);
        connect(m_serialInterface, &SerialInterface::lineReceived,
                this, &MainWindow::onSerialDataReceived);
        connect(m_serialInterface, &SerialInterface::errorOccurred,
                this, &MainWindow::onSerialError);
    }
    
    // 电机控制器信号
    if (m_motorController) {
        connect(m_motorController, &MotorController::statusChanged,
                this, &MainWindow::onMotorStatusChanged);
    }
    
    // 传感器管理器信号
    if (m_sensorManager) {
        connect(m_sensorManager, &SensorManager::dataUpdated,
                this, &MainWindow::onSensorDataUpdated);
    }
    
    // 日志信号
    connect(&Logger::instance(), &Logger::newLogEntry,
            this, &MainWindow::onLogMessage);
    
    // 更新定时器
    connect(m_updateTimer, &QTimer::timeout,
            this, &MainWindow::updateSensorDisplay);
}

void MainWindow::loadConfiguration() {
    ConfigManager& config = ConfigManager::instance();
    
    // 设置限制值
    ui->doubleSpinBox_3->setValue(config.getMaxHeight());
    ui->doubleSpinBox_8->setValue(config.getMinHeight());
    ui->doubleSpinBox_6->setValue(config.getMinAngle());
    ui->doubleSpinBox_7->setValue(config.getMaxAngle());
}

void MainWindow::updateUIState(bool connected) {
    // 串口通信页面
    ui->pushButton_3->setEnabled(!connected);  // Add Device
    ui->pushButton->setEnabled(connected);      // Disconnect
    ui->pushButton_2->setEnabled(connected);    // Send
    ui->lineEdit->setEnabled(connected);
    
    // 其他标签页
    ui->tabWidget->setTabEnabled(1, connected); // Motor Control
    ui->tabWidget->setTabEnabled(2, connected); // Sensor Monitor
    
    // 状态栏
    if (connected) {
        ui->statusbar->showMessage("Connected");
    } else {
        ui->statusbar->showMessage("Disconnected");
    }
}

// 串口通信页面槽函数
void MainWindow::onAddDeviceClicked() {
    auto currentItem = ui->listWidget->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "Warning", "Please select a device first");
        return;
    }
    
    QString text = currentItem->text();
    QString portName = text.split(" - ").first();
    
    if (m_serialInterface && m_serialInterface->connect(portName, 115200)) {
        ui->label_3->setText(portName);
        LOG_INFO(QString("Connected to %1").arg(portName));
    } else {
        QMessageBox::critical(this, "Error", "Failed to connect to device");
    }
}

void MainWindow::onDisconnectClicked() {
    if (m_serialInterface) {
        m_serialInterface->disconnect();
    }
}

void MainWindow::onSendCommandClicked() {
    QString command = ui->lineEdit->text();
    if (!command.isEmpty() && m_serialInterface) {
        if (m_serialInterface->sendCommand(command)) {
            ui->textEdit->append(QString("> %1").arg(command));
            ui->lineEdit->clear();
        }
    }
}

void MainWindow::onDeviceSelected() {
    // 设备被选中时的处理
}

void MainWindow::updatePortList() {
    ui->listWidget->clear();
    
    if (m_serialInterface) {
        auto ports = m_serialInterface->getAvailablePorts();
        for (const auto& port : ports) {
            QString item = QString("%1 - %2")
                          .arg(port.portName)
                          .arg(port.description);
            ui->listWidget->addItem(item);
        }
    }
    
    if (ui->listWidget->count() == 0) {
        ui->listWidget->addItem("No serial ports found");
    }
}

// 电机控制页面槽函数
void MainWindow::onSetHeightClicked() {
    float height = ui->doubleSpinBox->value();
    if (m_motorController) {
        m_motorController->setHeight(height);
    }
}

void MainWindow::onSetAngleClicked() {
    float angle = ui->doubleSpinBox_2->value();
    if (m_motorController) {
        m_motorController->setAngle(angle);
    }
}

void MainWindow::onMoveToPositionClicked() {
    float height = ui->doubleSpinBox->value();
    float angle = ui->doubleSpinBox_2->value();
    if (m_motorController) {
        m_motorController->moveToPosition(height, angle);
    }
}

void MainWindow::onStopMotorClicked() {
    if (m_motorController) {
        m_motorController->stop();
    }
}

void MainWindow::onHomePositionClicked() {
    if (m_motorController) {
        m_motorController->goHome();
    }
}

void MainWindow::onEmergencyStopClicked() {
    if (m_motorController) {
        m_motorController->emergencyStop();
    }
    QMessageBox::warning(this, "Emergency Stop", 
                        "Emergency stop activated! Reset system before continuing.");
}

void MainWindow::onAutoCalibateClicked() {
    // 实现自动校准
    QMessageBox::information(this, "Auto Calibration", 
                           "Auto calibration feature coming soon!");
}

void MainWindow::onManualCalibrateClicked() {
    if (!m_calibrationDialog) {
        m_calibrationDialog = std::make_unique<ManualCalibrationDialog>(this);
    }
    
    if (m_calibrationDialog->exec() == QDialog::Accepted) {
        // 应用校准值
        float actualHeight = m_calibrationDialog->getActualHeight();
        float actualAngle = m_calibrationDialog->getActualAngle();
        
        // TODO: 应用到校准引擎
        LOG_INFO(QString("Manual calibration: H=%1mm, A=%2°")
                .arg(actualHeight).arg(actualAngle));
    }
}

// 传感器监控页面槽函数
void MainWindow::onRecordDataClicked() {
    if (!m_dataRecorder) return;
    
    if (m_isRecording) {
        m_dataRecorder->stopRecording();
        ui->pushButton_13->setText("Record Data");
        m_isRecording = false;
    } else {
        if (m_dataRecorder->startRecording()) {
            ui->pushButton_13->setText("Stop Recording");
            m_isRecording = true;
        }
    }
}

void MainWindow::onExportDataClicked() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Data", "", "CSV Files (*.csv)");
    
    if (!fileName.isEmpty()) {
        // TODO: 实现数据导出
        QMessageBox::information(this, "Export", 
                               "Data export feature coming soon!");
    }
}

void MainWindow::updateSensorDisplay() {
    // 这个方法由定时器调用，更新传感器显示
    if (m_sensorManager) {
        // 获取最新数据并更新UI
        // 实际实现中应该从传感器管理器获取数据
    }
}

// 其他槽函数实现...
void MainWindow::onSerialConnected(const QString& port) {
    ui->statusbar->showMessage(QString("Connected to %1").arg(port));
    updateUIState(true);
    
    // 启动传感器监控
    if (m_sensorManager) {
        m_sensorManager->startMonitoring();
    }
}

void MainWindow::onSerialDisconnected() {
    ui->statusbar->showMessage("Disconnected");
    ui->label_3->setText("No device selected");
    updateUIState(false);
    
    // 停止传感器监控
    if (m_sensorManager) {
        m_sensorManager->stopMonitoring();
    }
}

void MainWindow::onSerialDataReceived(const QString& data) {
    ui->textEdit->append(QString("< %1").arg(data));
}

void MainWindow::onSerialError(const QString& error) {
    ui->statusbar->showMessage(QString("Error: %1").arg(error), 5000);
}

void MainWindow::onMotorStatusChanged() {
    if (!m_motorController) return;
    
    auto status = m_motorController->getStatus();
    
    // 更新电机状态显示
    ui->label_11->setText(QString("%1mm").arg(status.currentHeight, 0, 'f', 1));
    ui->label_8->setText(QString("%1°").arg(status.currentAngle, 0, 'f', 1));
    
    if (status.isMoving) {
        ui->label_19->setText("Motor Status: Moving");
    } else {
        ui->label_19->setText("Motor Status: Ready");
    }
}

void MainWindow::onSensorDataUpdated(const SensorData& data) {
    m_lastSensorData = data;
    
    // 更新传感器显示
    if (data.isValid.temperature) {
        ui->label_27->setText(QString("%1°C").arg(data.temperature, 0, 'f', 1));
    }
    
    if (data.isValid.angle) {
        ui->label_25->setText(QString("%1°").arg(data.angle, 0, 'f', 1));
    }
    
    if (data.isValid.distanceUpper1) {
        ui->label_23->setText(QString("%1mm").arg(data.calculatedHeight, 0, 'f', 1));
    }
    
    if (data.isValid.capacitance) {
        ui->label_30->setText(QString("%1 pF").arg(data.capacitance, 0, 'f', 1));
    }
    
    // 更新详细传感器数据
    ui->label_44->setText(QString("%1mm").arg(data.distanceUpper1, 0, 'f', 1));
    ui->label_45->setText(QString("%1mm").arg(data.distanceUpper2, 0, 'f', 1));
    ui->label_53->setText(QString("%1mm").arg(data.distanceLower1, 0, 'f', 1));
    ui->label_54->setText(QString("%1mm").arg(data.distanceLower2, 0, 'f', 1));
    
    // 更新时间戳
    ui->label_38->setText(data.timestamp.toString("hh:mm:ss"));
}

void MainWindow::onLogMessage(const QString& message) {
    ui->textEdit_2->append(message);
}

// 其余槽函数的简单实现
void MainWindow::onClearLogClicked() {
    ui->textEdit_2->clear();
}

void MainWindow::onBrowseFileClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "Select CSV File", "", "CSV Files (*.csv)");
    if (!fileName.isEmpty()) {
        ui->lineEdit_2->setText(fileName);
    }
}

void MainWindow::onLoadDataClicked() {
    QString fileName = ui->lineEdit_2->text();
    if (!fileName.isEmpty()) {
        // TODO: 实现数据加载
    }
}

void MainWindow::updateLogFilter() {
    // TODO: 实现日志过滤
}

void MainWindow::onChartTypeChanged() {
    // TODO: 更新图表类型
}

void MainWindow::onLoadCSVClicked() {
    // TODO: 加载CSV文件到图表
}

void MainWindow::onStartRealtimeClicked() {
    // TODO: 启动实时图表更新
}