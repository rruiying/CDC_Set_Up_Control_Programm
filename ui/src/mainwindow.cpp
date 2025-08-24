#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../../src/app/include/application_controller.h"
#include "../include/adddevicedialog.h"
#include "../include/confirmdialog.h"
#include "../include/errordialog.h"
#include "../../src/models/include/device_info.h"
#include "../../src/utils/include/logger.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QCloseEvent>
#include <QTextStream>
#include <QDateTime>
#include <QScrollBar>
#include <QTimer>
#include <QGroupBox>
#include <QResizeEvent>
#include <QSplitter>
#include <QDebug>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QBrush>
#include <QColor>


MainWindow::MainWindow(ApplicationController* controller, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_controller(controller)
    , currentSelectedDeviceIndex(-1)
    , isInitialized(false)
    , targetHeight(0.0)
    , targetAngle(0.0)
    , currentHeight(0.0)
    , currentAngle(0.0)
    , theoreticalCapacitance(0.0)
    , logUpdateTimer(new QTimer(this))
    , sensorUpdateTimer(new QTimer(this))
    , currentLogLevel(LogLevel::ALL)
    , lastDisplayedLogCount(0)
{
    ui->setupUi(this);

    setWindowTitle("CDC_Control_Programm v1.0");
    setMinimumSize(800, 600);

    initializeDeviceManagement();
    initializeMotorControl();
    initializeSensorMonitor();
    initializeLogViewer();

    if (m_controller) {
        setupCallbacks();
    }
    
    isInitialized = true;

    logUserOperation("Application started");
    showStatusMessage("CDC Control System Started");
}

MainWindow::~MainWindow() {
    if (logUpdateTimer) {
        logUpdateTimer->stop();
        delete logUpdateTimer;
    }
    if (sensorUpdateTimer) {
        sensorUpdateTimer->stop();
        delete sensorUpdateTimer;
    }
    
    if (m_controller) {
        m_controller->stopSensorMonitoring();
    }
    
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // 检查是否有设备连接
    bool hasConnectedDevices = false;
    if (m_controller) {
        auto devices = m_controller->getDeviceList();
        for (const auto& device : devices) {
            if (device.connectionStatus == 2) { // CONNECTED
                hasConnectedDevices = true;
                break;
            }
        }
    }
    
    if (hasConnectedDevices) {
        if (ConfirmDialog::confirm(this,
                                  "There are still connected devices. Are you sure you want to exit?",
                                  "Confirm Exit",
                                  "Exit",
                                  "Cancel")) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

// ===== 第一页：设备管理UI初始化 =====

void MainWindow::initializeDeviceManagement()
{
    ui->tabWidget->setCurrentIndex(0);
    
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::onAddDeviceClicked);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::onRemoveDeviceClicked);
    connect(ui->pushButton_20, &QPushButton::clicked, this, &MainWindow::onConnectDeviceClicked);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onDisconnectDeviceClicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::onSendCommandClicked);

    connect(ui->listWidget, &QListWidget::currentItemChanged,
            this, &MainWindow::onDeviceSelectionChanged);
    
    connect(ui->lineEdit, &QLineEdit::returnPressed, 
            this, &MainWindow::onCommandLineReturnPressed);
    
    updateDeviceListDisplay();
    updateSelectedDeviceDisplay();
    updateDeviceButtons();
    clearCommunicationLog();
    
    logUserOperation("Device management initialized");
}

// ===== 设备管理槽函数实现 =====

void MainWindow::onAddDeviceClicked() {
    if (!m_controller) return;
    QStringList existingNames = getExistingDeviceNames();

    auto availablePorts = m_controller->getAvailablePorts();
    QList<DeviceInfo> emptyList;

    for (const auto& port : availablePorts) {
        DeviceInfo info("", port, 115200);
        emptyList.append(info);
    }
    
    AddDeviceDialog dialog(this, existingNames, emptyList);
    
    if (dialog.exec() == QDialog::Accepted) {
        DeviceInfo newDevice = dialog.getDeviceInfo();

        if (m_controller->addDevice(
                newDevice.getName(),
                newDevice.getPortName(),
                newDevice.getBaudRate())) {
            updateDeviceListDisplay();
            logUserOperation(QString("Device added: %1 on %2")
                            .arg(QString::fromStdString(newDevice.getName()))
                            .arg(QString::fromStdString(newDevice.getPortName())));
            showStatusMessage(QString("Device '%1' added")
                            .arg(QString::fromStdString(newDevice.getName())));
        } else {
            ErrorDialog::showError(this, ErrorDialog::DataValidationError,
                                 "Failed to add device");
        }
    }
}

void MainWindow::onRemoveDeviceClicked() {
    if (!m_controller || currentSelectedDeviceIndex < 0) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, 
                             "Please select a device to remove");
        return;
    }
    
    auto devices = m_controller->getDeviceList();
    if (currentSelectedDeviceIndex >= devices.size()) return;
    
    QString deviceName = QString::fromStdString(devices[currentSelectedDeviceIndex].name);
    
    if (devices[currentSelectedDeviceIndex].connectionStatus == 2) {
        if (!ConfirmDialog::confirmDisconnectDevice(this, deviceName)) {
            return;
        }
        m_controller->disconnectDevice(currentSelectedDeviceIndex);
    }
    
    if (ConfirmDialog::confirmDeleteDevice(this, deviceName)) {
        if (m_controller->removeDevice(currentSelectedDeviceIndex)) {
            currentSelectedDeviceIndex = -1;
            updateDeviceListDisplay();
            updateSelectedDeviceDisplay();
            updateDeviceButtons();
            logUserOperation(QString("Device removed: %1").arg(deviceName));
            showStatusMessage(QString("Device '%1' removed").arg(deviceName));
        }
    }
}

void MainWindow::onConnectDeviceClicked() {
    try {
        Logger::getInstance().info("=== Connect button clicked ===");

        if (!m_controller) {
            Logger::getInstance().error("Controller is NULL!");
            ErrorDialog::showError(this, ErrorDialog::DataValidationError,
                                 "Controller not initialized");
            return;
        }

        Logger::getInstance().info("Current selected device index: " + std::to_string(currentSelectedDeviceIndex));

        if (currentSelectedDeviceIndex < 0) {
            ErrorDialog::showError(this, ErrorDialog::DataValidationError,
                                 "Please select a device to connect");
            return;
        }
        auto devices = m_controller->getDeviceList();
        Logger::getInstance().info("Device list size: " + std::to_string(devices.size()));
        if (currentSelectedDeviceIndex >= devices.size()) {
            Logger::getInstance().error("Device index out of range");
            return;
        }

        Logger::getInstance().info("Device name: " + devices[currentSelectedDeviceIndex].name);
        Logger::getInstance().info("Device port: " + devices[currentSelectedDeviceIndex].portName);
        Logger::getInstance().info("Device status: " + std::to_string(devices[currentSelectedDeviceIndex].connectionStatus));
    
        if (devices[currentSelectedDeviceIndex].connectionStatus == 2) {
            showStatusMessage("Device already connected");
            return;
        }
    
        Logger::getInstance().info("Calling m_controller->connectDevice()...");
        bool result = m_controller->connectDevice(currentSelectedDeviceIndex);
        Logger::getInstance().info("connectDevice returned: " + std::string(result ? "true" : "false"));

        if (result) {
            QString deviceName = QString::fromStdString(devices[currentSelectedDeviceIndex].name);
            showStatusMessage(QString("Connecting to %1...").arg(deviceName));
            ui->textEdit->clear();
            addCommunicationLog(QString("=== Connecting to device: %1 ===").arg(deviceName), false);
            setupRawSerialCommunication();
        } else {
            ErrorDialog::showError(this, ErrorDialog::CommunicationError,
                                 "Failed to connect to device");
        }
    
        updateDeviceListDisplay();
        updateSelectedDeviceDisplay();
        updateDeviceButtons();
        Logger::getInstance().info("=== Connect function completed ===");
    } catch (const std::exception& e) {
        Logger::getInstance().error("Exception in onConnectDeviceClicked: " + std::string(e.what()));
        QMessageBox::critical(this, "Error", QString("Exception: %1").arg(e.what()));
    } catch (...) {
        Logger::getInstance().error("Unknown exception in onConnectDeviceClicked");
        QMessageBox::critical(this, "Error", "Unknown exception occurred");
    }
}

void MainWindow::onDisconnectDeviceClicked() {
    if (!m_controller || currentSelectedDeviceIndex < 0) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError,
                             "Please select a device to disconnect");
        return;
    }
    
    auto devices = m_controller->getDeviceList();
    if (currentSelectedDeviceIndex >= devices.size()) return;
    
    QString deviceName = QString::fromStdString(devices[currentSelectedDeviceIndex].name);
    
    if (ConfirmDialog::confirmDisconnectDevice(this, deviceName)) {
        if (m_controller->disconnectDevice(currentSelectedDeviceIndex)) {
            showStatusMessage(QString("Device '%1' disconnected").arg(deviceName));
            addCommunicationLog(QString("=== Disconnected from device: %1 ===").arg(deviceName), false);
        }
        updateDeviceListDisplay();
        updateSelectedDeviceDisplay();
        updateDeviceButtons();
    }
}

void MainWindow::onSendCommandClicked() {
    if (!m_controller) return;
    
    QString command = ui->lineEdit->text();
    if (command.isEmpty()) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError,
                             "Please enter a command to send");
        return;
    }

    addCommunicationLog(command, true);
    sendCommandToCurrentDevice(command);
    //addToCommandHistory(command);
    ui->lineEdit->clear();
    ui->lineEdit->setFocus();
}

void MainWindow::onDeviceSelectionChanged(QListWidgetItem *current, QListWidgetItem *previous) {
    Q_UNUSED(previous)
    
    if (current) {
        currentSelectedDeviceIndex = ui->listWidget->row(current);
    } else {
        currentSelectedDeviceIndex = -1;
    }
    
    updateSelectedDeviceDisplay();
    updateDeviceButtons();
}

void MainWindow::onCommandLineReturnPressed() {
    onSendCommandClicked();
}

// ===== 第二页：电机控制UI初始化 =====

void MainWindow::initializeMotorControl() {
    ui->doubleSpinBox_3->setValue(200.0);
    ui->doubleSpinBox_8->setValue(0.0);
    ui->doubleSpinBox_6->setValue(-90.0);
    ui->doubleSpinBox_7->setValue(90.0);

    ui->doubleSpinBox->setRange(0.0, 200.0);
    ui->doubleSpinBox->setValue(0.0);
    ui->doubleSpinBox_2->setRange(-90.0, 90.0);
    ui->doubleSpinBox_2->setValue(0.0);    

    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::onSetHeightClicked);
    connect(ui->pushButton_9, &QPushButton::clicked, this, &MainWindow::onSetAngleClicked);
    connect(ui->pushButton_7, &QPushButton::clicked, this, &MainWindow::onMoveToPositionClicked);
    connect(ui->pushButton_8, &QPushButton::clicked, this, &MainWindow::onHomePositionClicked);
    connect(ui->pushButton_6, &QPushButton::clicked, this, &MainWindow::onStopMotorClicked);
    connect(ui->pushButton_12, &QPushButton::clicked, this, &MainWindow::onEmergencyStopClicked);
    
    // 安全限位变化信号
    connect(ui->doubleSpinBox_3, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSafetyLimitsChanged);
    connect(ui->doubleSpinBox_8, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSafetyLimitsChanged);
    connect(ui->doubleSpinBox_6, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSafetyLimitsChanged);
    connect(ui->doubleSpinBox_7, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSafetyLimitsChanged);
        
    updateMotorControlDisplay();
    updateMotorControlButtons();
    
    logUserOperation("Motor control initialized");
}

// ===== 第二页：电机控制槽函数实现 =====

void MainWindow::onSetHeightClicked() {
    targetHeight = ui->doubleSpinBox->value();
    
    if (!checkSafetyLimits(targetHeight, targetAngle)) {
        return;
    }
    
    logUserOperation(QString("Target height set to %1 mm").arg(targetHeight, 0, 'f', 1));
    showStatusMessage(QString("Target height set to %1 mm").arg(targetHeight, 0, 'f', 1));
}

void MainWindow::onSetAngleClicked() {
    targetAngle = ui->doubleSpinBox_2->value();
    
    if (!checkSafetyLimits(targetHeight, targetAngle)) {
        return;
    }

    logUserOperation(QString("Target angle set to %1°").arg(targetAngle, 0, 'f', 1));
    showStatusMessage(QString("Target angle set to %1°").arg(targetAngle, 0, 'f', 1));
}

void MainWindow::onMoveToPositionClicked() {
    if (!m_controller) return;
    
    auto device = m_controller->getCurrentDevice();
    if (device.connectionStatus != 2) {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError,
                             "Please connect a device first");
        return;
    }
    
    if (!checkSafetyLimits(targetHeight, targetAngle)) {
        return;
    }
    
    if (m_controller->moveToPosition(targetHeight, targetAngle)) {
        currentHeight = targetHeight;
        currentAngle = targetAngle;
        updateTheoreticalCapacitance();
        updateMotorControlDisplay();

        logUserOperation(QString("Move command sent: height=%1 mm, angle=%2°")
                       .arg(targetHeight, 0, 'f', 1).arg(targetAngle, 0, 'f', 1));
        showStatusMessage(QString("Moving to: %1 mm, %2°")
                        .arg(targetHeight, 0, 'f', 1).arg(targetAngle, 0, 'f', 1));
    } else {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError,
                             "Failed to send move command");
    }
}

void MainWindow::onHomePositionClicked() {
    if (!m_controller) return;
    
    auto device = m_controller->getCurrentDevice();
    if (device.connectionStatus != 2) {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError,
                             "Please connect a device first");
        return;
    }
    
    if (m_controller->homeMotor()) {
        currentHeight = 0.0;
        currentAngle = 0.0;
        targetHeight = 0.0;
        targetAngle = 0.0;
        
        ui->doubleSpinBox->setValue(0.0);
        ui->doubleSpinBox_2->setValue(0.0);

        ui->pushButton_8->setStyleSheet("");
        
        updateTheoreticalCapacitance();
        updateMotorControlDisplay();
        updateMotorControlButtons();
        
        logUserOperation("Home position - System reset from emergency stop");
        showStatusMessage("System reset - Ready for operation");
    } else {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError,
                             "Failed to send home command");
    }
}

void MainWindow::onStopMotorClicked() {
    if (!m_controller) return;
    
    if (m_controller->stopMotor()) {
        logUserOperation("Motor stop command sent");
        showStatusMessage("Motor stopped");
    } else {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError,
                             "Failed to send stop command");
    }
}

void MainWindow::onEmergencyStopClicked() {
    if (!m_controller) return;
    
    m_controller->emergencyStop();

    ui->pushButton_5->setEnabled(false);
    ui->pushButton_9->setEnabled(false);
    ui->pushButton_7->setEnabled(false);
    ui->pushButton_6->setEnabled(false);

    ui->pushButton_8->setEnabled(true);
    ui->pushButton_8->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }"
    );

    ui->pushButton_12->setEnabled(true);
    
    logUserOperation("EMERGENCY STOP activated");
    showStatusMessage("EMERGENCY STOP - Press HOME button to reset", 10000);
}

void MainWindow::onSafetyLimitsChanged() {
    if (!m_controller) return;
    
    double maxHeight = ui->doubleSpinBox_3->value();
    double minHeight = ui->doubleSpinBox_8->value();
    double minAngle = ui->doubleSpinBox_6->value();
    double maxAngle = ui->doubleSpinBox_7->value();
    
    if (minHeight >= maxHeight) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError,
                             "Min height must be less than max height");
        return;
    }
    
    if (minAngle >= maxAngle) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError,
                             "Min angle must be less than max angle");
        return;
    }
    
    m_controller->setSafetyLimits(minHeight, maxHeight, minAngle, maxAngle);
    updateTargetPositionRanges();
    
    logUserOperation("Safety limits updated");
    showStatusMessage("Safety limits updated");
}

// ===== 第二页：辅助方法实现 =====

void MainWindow::updateMotorControlDisplay() {
    if (!m_controller) return;
    
    ui->label_11->setText(QString("%1 mm").arg(currentHeight, 0, 'f', 1));
    ui->label_8->setText(QString("%1°").arg(currentAngle, 0, 'f', 1));
    ui->label_13->setText(QString("%1 pF").arg(theoreticalCapacitance, 0, 'f', 1));

    QString motorStatus = "Ready";
    int status = m_controller->getMotorStatus();
    switch (status) {
        case 1: motorStatus = "Moving"; break;
        case 2: motorStatus = "Error"; break;
        case 3: motorStatus = "Homing"; break;
        case 4: motorStatus = "Calibrating"; break;
    }
    
    if (ui->label_19) {
        ui->label_19->setText(QString("Motor: %1").arg(motorStatus));
    }
    
    auto device = m_controller->getCurrentDevice();
    QString connStatus = device.connectionStatus == 2 ? "Connected" : "Disconnected";
    if (ui->label_20) {
        ui->label_20->setText(QString("Status: %1").arg(connStatus));
    }
}

void MainWindow::updateMotorControlButtons() {
    if (!m_controller) return;
    
    auto device = m_controller->getCurrentDevice();
    bool isConnected = (device.connectionStatus == 2);
    int motorStatus = m_controller->getMotorStatus();
    bool isMoving = (motorStatus == 1 || motorStatus == 3);
    
    if (m_controller->isEmergencyStopped()) {
        ui->pushButton_5->setEnabled(false);
        ui->pushButton_9->setEnabled(false);
        ui->pushButton_7->setEnabled(false);
        ui->pushButton_6->setEnabled(false);
        
        ui->pushButton_8->setEnabled(true);
        ui->pushButton_8->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }"
        );
        
        ui->pushButton_12->setEnabled(true);
    } else {
        ui->pushButton_5->setEnabled(true);
        ui->pushButton_9->setEnabled(true);
        ui->pushButton_7->setEnabled(isConnected);
        ui->pushButton_8->setEnabled(isConnected);
        ui->pushButton_8->setStyleSheet("");
        ui->pushButton_6->setEnabled(isConnected && isMoving);
        ui->pushButton_12->setEnabled(true);
    }
}

bool MainWindow::checkSafetyLimits(double height, double angle) {
    if (!m_controller) return false;
    
    if (!m_controller->checkSafetyLimits(height, angle)) {
        QString message = QString("Position exceeds safety limits!\n")
                        + QString("Height: %1 mm\nAngle: %2°")
                          .arg(height, 0, 'f', 1).arg(angle, 0, 'f', 1);
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, message);
        return false;
    }
    return true;
}

void MainWindow::updateTargetPositionRanges() {
    double maxHeight = ui->doubleSpinBox_3->value();
    double minHeight = ui->doubleSpinBox_8->value();
    double minAngle = ui->doubleSpinBox_6->value();
    double maxAngle = ui->doubleSpinBox_7->value();
    
    ui->doubleSpinBox->setRange(minHeight, maxHeight);
    ui->doubleSpinBox_2->setRange(minAngle, maxAngle);
}

void MainWindow::updateTheoreticalCapacitance() {
    if (m_controller) {
        theoreticalCapacitance = m_controller->calculateTheoreticalCapacitance(
            currentHeight, currentAngle);
    }
}

// ===== 第三页：传感器监控UI初始化 =====

void MainWindow::initializeSensorMonitor() {
    connect(ui->pushButton_13, &QPushButton::clicked, this, &MainWindow::onRecordDataClicked);
    connect(ui->pushButton_14, &QPushButton::clicked, this, &MainWindow::onExportDataClicked);
    
    //connect(sensorUpdateTimer, &QTimer::timeout, this, &MainWindow::onSensorUpdateTimer);
    //sensorUpdateTimer->start(500);
    
    updateSensorMonitorDisplay();
    
    logUserOperation("Sensor monitor initialized");
}

// ===== 第三页：传感器监控槽函数实现 =====

void MainWindow::onRecordDataClicked() {
    Logger::getInstance().info("=== Record Data clicked ===");
    if (!m_controller) {
        Logger::getInstance().error("Controller is null");
        return;
    }

    showStatusMessage("Getting sensor data...");
    ui->pushButton_13->setEnabled(false);

    bool success = m_controller->updateSensorData();
    
    if (success) {
        updateSensorMonitorDisplay();
        if (m_controller->recordCurrentData()) {
            lastRecordTime = QDateTime::currentDateTime();
            ui->label_38->setText(lastRecordTime.toString("hh:mm:ss"));
            
            size_t count = m_controller->getRecordCount();
            showStatusMessage(QString("Data recorded (Total: %1)").arg(count));
            
            logUserOperation(QString("Sensor data recorded at %1").arg(
                lastRecordTime.toString("hh:mm:ss")));
        }
    } else {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError,
                             "Failed to get valid sensor data");
    }
    
    ui->pushButton_13->setEnabled(true);
}

void MainWindow::onExportDataClicked() {
    if (!m_controller) return;
    
    if (m_controller->getRecordCount() == 0) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError,
                             "No data available for export");
        return;
    }
    
    QString defaultFilename = QString::fromStdString(m_controller->generateDefaultFilename());
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Export Measurement Data",
        defaultFilename,
        "CSV Files (*.csv);;All Files (*.*)"
    );
    
    if (filename.isEmpty()) return;
    
    if (m_controller->exportToCSV(filename.toStdString())) {
        QMessageBox::information(this, "Export Successful",
                               QString("Data exported to %1").arg(filename));
        logUserOperation(QString("Data exported to %1").arg(filename));
    } else {
        ErrorDialog::showError(this, ErrorDialog::FileOperationError,
                             "Failed to export data");
    }
}

//void MainWindow::onSensorUpdateTimer() {
//    if (!m_controller) return;
    
//    currentHeight = m_controller->getCurrentHeight();
//    currentAngle = m_controller->getCurrentAngle();
//    updateSensorMonitorDisplay();
//}

void MainWindow::updateSensorMonitorDisplay() {
    if (!m_controller) return;
    
    std::string jsonData = m_controller->getCurrentSensorDataJson();
    if (jsonData == "{}") {
        ui->label_23->setText("--");
        ui->label_25->setText("--");
        ui->label_27->setText("--");
        ui->label_30->setText("--");
        ui->label_32->setText("--");
        ui->label_34->setText("--");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(jsonData));
    QJsonObject obj = doc.object();
    
    double upper1 = parseJsonValue(obj["distanceUpper1"]);
    double upper2 = parseJsonValue(obj["distanceUpper2"]);
    double lower1 = parseJsonValue(obj["distanceLower1"]);
    double lower2 = parseJsonValue(obj["distanceLower2"]);
    double temperature = parseJsonValue(obj["temperature"]);
    double angle = parseJsonValue(obj["angle"]);
    double capacitance = parseJsonValue(obj["capacitance"]);
    double height = parseJsonValue(obj["height"]);
    
    ui->label_44->setText(formatSensorValue(upper1, 1, " mm"));
    ui->label_45->setText(formatSensorValue(upper2, 1, " mm"));
    ui->label_53->setText(formatSensorValue(lower1, 1, " mm"));
    ui->label_54->setText(formatSensorValue(lower2, 1, " mm"));
    ui->label_23->setText(formatSensorValue(height, 1, " mm"));
    ui->label_25->setText(formatSensorValue(angle, 1, "°"));
    ui->label_27->setText(formatSensorValue(temperature, 1, "°C"));
    ui->label_30->setText(formatSensorValue(capacitance, 1, " pF"));
    
    setLabelStyle(ui->label_44, upper1);
    setLabelStyle(ui->label_45, upper2);
    setLabelStyle(ui->label_53, lower1);
    setLabelStyle(ui->label_54, lower2);
    setLabelStyle(ui->label_23, height);
    setLabelStyle(ui->label_25, angle);
    setLabelStyle(ui->label_27, temperature);
    setLabelStyle(ui->label_30, capacitance);
    
    double avgGround = (lower1 + lower2) / 2.0;
    ui->label_56->setText(formatSensorValue(avgGround, 1, " mm"));
    setLabelStyle(ui->label_56, avgGround);
    
    double theoretical = calculateTheoreticalCapacitance();
    ui->label_32->setText(formatSensorValue(theoretical, 1, " pF"));
    ui->label_34->setText(formatSensorValue(capacitance - theoretical, 1, " pF"));
}

double MainWindow::calculateTheoreticalCapacitance() const {
    if (m_controller) {
        return m_controller->calculateTheoreticalCapacitance(
            currentHeight, currentAngle);
    }
    return 0.0;
}

QString MainWindow::formatSensorValue(double value, int precision, const QString& suffix) {
    if (std::isnan(value)) {
        return "NaN";
    } else if (std::isinf(value)) {
        return value > 0 ? "+∞" : "-∞";
    } else {
        return QString("%1%2").arg(value, 0, 'f', precision).arg(suffix);
    }
}

void MainWindow::setLabelStyle(QLabel* label, double value) {
    if (std::isnan(value)) {
        label->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        label->setToolTip("Not a Number - Sensor error");
    } else if (std::isinf(value)) {
        label->setStyleSheet("QLabel { color: orange; font-weight: bold; }");
        label->setToolTip("Infinity - Out of range");
    } else {
        label->setStyleSheet("");
        label->setToolTip("");
    }
}

// ===== 第四页：日志查看UI初始化 =====

void MainWindow::initializeLogViewer() {
    connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLogLevelChanged);
    connect(ui->pushButton_15, &QPushButton::clicked, this, &MainWindow::onClearLogClicked);
    connect(ui->pushButton_10, &QPushButton::clicked, this, &MainWindow::onSaveLogClicked);
    
    ui->comboBox_2->clear();
    ui->comboBox_2->addItem("All", 0);
    ui->comboBox_2->addItem("Info", 1);
    ui->comboBox_2->addItem("Warning", 2);
    ui->comboBox_2->addItem("Error", 3);
    ui->comboBox_2->setCurrentIndex(0);
    
    ui->textEdit_2->setReadOnly(true);
    ui->textEdit_2->document()->setMaximumBlockCount(MAX_LOG_DISPLAY_LINES);
    
    connect(logUpdateTimer, &QTimer::timeout, this, &MainWindow::onLogUpdateTimer);
    logUpdateTimer->start(500);
    
    updateLogDisplay();
    
    logUserOperation("Log viewer initialized");
}

// ===== 第四页：日志查看槽函数实现 =====

void MainWindow::onLogLevelChanged(int index) {
    currentLogLevel = static_cast<LogLevel>(ui->comboBox_2->itemData(index).toInt());
    updateLogDisplay();
    showStatusMessage(QString("Log level: %1").arg(ui->comboBox_2->currentText()));
}

void MainWindow::onClearLogClicked() {
    if (!m_controller) return;
    
    if (ConfirmDialog::confirm(this,
                              "Clear all logs?",
                              "Confirm Clear",
                              "Clear",
                              "Cancel")) {
        m_controller->clearLogs();
        ui->textEdit_2->clear();
        lastDisplayedLogCount = 0;
        logUserOperation("Logs cleared");
        showStatusMessage("Logs cleared");
    }
}

void MainWindow::onSaveLogClicked() {
    if (!m_controller) return;
    
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Save Logs",
        generateLogFilename(),
        "Text Files (*.txt);;Log Files (*.log);;All Files (*.*)"
    );
    
    if (filename.isEmpty()) return;
    
    if (saveLogsToFile(filename)) {
        QMessageBox::information(this, "Save Successful",
                               QString("Logs saved to %1").arg(filename));
        logUserOperation(QString("Logs saved to %1").arg(filename));
    } else {
        ErrorDialog::showError(this, ErrorDialog::FileOperationError,
                             "Failed to save logs");
    }
}

void MainWindow::onLogUpdateTimer() {
    updateLogDisplay();
}

// ===== 第四页：辅助方法实现 =====

void MainWindow::updateLogDisplay() {
    if (!m_controller) return;
    
    std::string logsJson = m_controller->getRecentLogsJson(100);
    
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(logsJson));
    QJsonArray logs = doc.array();
    
    if (logs.size() > lastDisplayedLogCount) {
        for (int i = lastDisplayedLogCount; i < logs.size(); ++i) {
            QJsonObject log = logs[i].toObject();
            int level = log["level"].toInt();
            
            if (static_cast<int>(currentLogLevel) == 0 || 
                level >= static_cast<int>(currentLogLevel)) {
                
                QString logLine = formatLogEntry(level, 
                    QString("[%1] %2")
                    .arg(log["time"].toString())
                    .arg(log["message"].toString()));
                
                ui->textEdit_2->append(logLine);
            }
        }
        lastDisplayedLogCount = logs.size();
    }
}

QString MainWindow::formatLogEntry(int level, const QString& message) const {
    QColor color;
    QString levelStr;
    
    switch (level) {
        case 3:
            color = QColor(220, 50, 50);
            levelStr = "ERROR";
            break;
        case 2:
            color = QColor(255, 140, 0);
            levelStr = "WARN";
            break;
        case 1:
            color = QColor(70, 130, 180);
            levelStr = "INFO";
            break;
        default:
            color = QColor(0, 0, 0);
            levelStr = "DEBUG";
            break;
    }
    
    return QString("[%1] %2").arg(levelStr).arg(message);
}

QString MainWindow::generateLogFilename() const {
    return QString("cdc_log_%1.txt")
           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
}

bool MainWindow::saveLogsToFile(const QString& filename) const {
    if (!m_controller) return false;
    return m_controller->saveLogsToFile(filename.toStdString());
}

void MainWindow::forceLogUpdate() {
    // 强制立即更新日志显示（在重要操作后调用）
    updateLogDisplay();
}

// ===== 修改现有方法，添加强制日志更新 =====

void MainWindow::logUserOperation(const QString& operation) {
    if (m_controller) {
        m_controller->logOperation(operation.toStdString());
    }
    
    if (isInitialized) {
        QTimer::singleShot(50, this, &MainWindow::forceLogUpdate);
    }
}
// ============================================
// 添加响应窗口大小变化的函数
// ============================================
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}

void MainWindow::setupCallbacks() {
   if (!m_controller) return;
    
    m_controller->setConnectionCallback(
        [this](bool connected, std::string device) {
            QMetaObject::invokeMethod(this, [this, connected, device]() {
                handleConnectionChanged(connected, QString::fromStdString(device));
            }, Qt::QueuedConnection);
        });
    
    m_controller->setDataCallback(
        [this](std::string data) {
            QMetaObject::invokeMethod(this, [this, data]() {
                handleDataReceived(QString::fromStdString(data));
            }, Qt::QueuedConnection);
        });
    
    m_controller->setSensorCallback(
        [this](std::string jsonData) {
            QMetaObject::invokeMethod(this, [this, jsonData]() {
                handleSensorData(jsonData);
            }, Qt::QueuedConnection);
        });
    
    m_controller->setMotorCallback(
        [this](int status) {
            QMetaObject::invokeMethod(this, [this, status]() {
                handleMotorStatusChanged(status);
            }, Qt::QueuedConnection);
        });
    
    m_controller->setErrorCallback(
        [this](std::string error) {
            QMetaObject::invokeMethod(this, [this, error]() {
                handleError(QString::fromStdString(error));
            }, Qt::QueuedConnection);
        });
}

void MainWindow::updateDeviceListDisplay() {
    if (!m_controller) return;
    
    ui->listWidget->clear();
    
    auto devices = m_controller->getDeviceList();
    for (size_t i = 0; i < devices.size(); ++i) {
        QString displayText = getDeviceDisplayText(i);
        QListWidgetItem* item = new QListWidgetItem(displayText);
        
        switch (devices[i].connectionStatus) {
            case 2:
                item->setForeground(QBrush(QColor(0, 128, 0)));
                break;
            case 1:
                item->setForeground(QBrush(QColor(255, 165, 0)));
                break;
            case 3:
                item->setForeground(QBrush(QColor(255, 0, 0)));
                break;
            default:
                item->setForeground(QBrush(QColor(0, 0, 0)));
                break;
        }
        
        ui->listWidget->addItem(item);
    }
    
    if (currentSelectedDeviceIndex >= 0 && currentSelectedDeviceIndex < devices.size()) {
        ui->listWidget->setCurrentRow(currentSelectedDeviceIndex);
    }
}

void MainWindow::updateSelectedDeviceDisplay() {
    if (!m_controller || currentSelectedDeviceIndex < 0) {
        ui->label_3->setText("No device selected");
        return;
    }
    
    auto devices = m_controller->getDeviceList();
    if (currentSelectedDeviceIndex < devices.size()) {
        auto& device = devices[currentSelectedDeviceIndex];
        QString status = device.connectionStatus == 2 ? "Connected" : 
                        device.connectionStatus == 1 ? "Connecting" :
                        device.connectionStatus == 3 ? "Error" : "Disconnected";
        ui->label_3->setText(QString("%1 (%2)")
                           .arg(QString::fromStdString(device.name))
                           .arg(status));
    }
}

void MainWindow::updateDeviceButtons() {
    if (!m_controller) {
        ui->pushButton_3->setEnabled(false);
        ui->pushButton_4->setEnabled(false);
        ui->pushButton_20->setEnabled(false);
        ui->pushButton->setEnabled(false);
        ui->pushButton_2->setEnabled(false);
        ui->lineEdit->setEnabled(false);
        return;
    }
    
    bool hasSelection = currentSelectedDeviceIndex >= 0;
    bool isConnected = false;
    
    if (hasSelection) {
        auto devices = m_controller->getDeviceList();
        if (currentSelectedDeviceIndex < devices.size()) {
            isConnected = (devices[currentSelectedDeviceIndex].connectionStatus == 2);
        }
    }
    
    ui->pushButton_3->setEnabled(true);  // Add always enabled
    ui->pushButton_4->setEnabled(hasSelection);  // Remove
    ui->pushButton_20->setEnabled(hasSelection && !isConnected);  // Connect
    ui->pushButton->setEnabled(hasSelection && isConnected);  // Disconnect
    ui->pushButton_2->setEnabled(isConnected);  // Send command
    ui->lineEdit->setEnabled(isConnected);
}

void MainWindow::clearCommunicationLog() {
    ui->textEdit->clear();
    addCommunicationLog("=== Communication log started ===", false);
}

void MainWindow::showStatusMessage(const QString& message, int timeout) {
    if (ui->statusbar) {
        ui->statusbar->showMessage(message, timeout);
    }
}

void MainWindow::addCommunicationLog(const QString& message, bool isOutgoing) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString prefix = isOutgoing ? "TX" : "RX";
    QString logLine = QString("[%1] %2: %3").arg(timestamp, prefix, message);
    
    ui->textEdit->append(logLine);
    
    if (ui->textEdit->document()->lineCount() > MAX_COMM_LOG_LINES) {
        QTextCursor cursor = ui->textEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 100);
        cursor.removeSelectedText();
    }
}

QStringList MainWindow::getExistingDeviceNames() const {
    QStringList names;
    if (m_controller) {
        auto devices = m_controller->getDeviceList();
        for (const auto& device : devices) {
            names << QString::fromStdString(device.name);
        }
    }
    return names;
}

QString MainWindow::getDeviceDisplayText(int index) const {
    if (!m_controller) return "";
    
    auto devices = m_controller->getDeviceList();
    if (index >= devices.size()) return "";
    
    auto& device = devices[index];
    QString status = device.connectionStatus == 2 ? "Connected" :
                    device.connectionStatus == 1 ? "Connecting" :
                    device.connectionStatus == 3 ? "Error" : "Disconnected";
    
    return QString("%1 [%2] - %3")
           .arg(QString::fromStdString(device.name))
           .arg(QString::fromStdString(device.portName))
           .arg(status);
}

bool MainWindow::isPortInUse(const QString& port) const {
    if (!m_controller) return false;
    return m_controller->isPortInUse(port.toStdString());
}

void MainWindow::sendCommandToCurrentDevice(const QString& command) {
    if (!m_controller) return;

    auto devices = m_controller->getDeviceList();
    bool hasConnectedDevice = false;

    for (const auto& device : devices) {
        if (device.connectionStatus == 2) {
            hasConnectedDevice = true;
            break;
        }
    }

    if (!hasConnectedDevice) {
        showStatusMessage("No device connected");
        addCommunicationLog("Error: No device connected", false);
        return;
    }

    std::string fullCommand = command.toStdString();
    if (!command.endsWith("\r\n")) {
        fullCommand += "\r\n";
    }
    
    bool success = m_controller->sendCommand(fullCommand);

    if (!success) {
        addCommunicationLog("Failed to send command", false);
        showStatusMessage("Failed to send command");
    }
}

void MainWindow::handleConnectionChanged(bool connected, const QString& device) {
    if (connected) {
        showStatusMessage(QString("Device '%1' connected").arg(device));
        
        //if (m_controller) {
        //    m_controller->startSensorMonitoring();
        //}
    } else {
        showStatusMessage(QString("Device '%1' disconnected").arg(device));
        
        if (m_controller) {
            m_controller->stopSensorMonitoring();
        }
    }
    
    updateDeviceListDisplay();
    updateSelectedDeviceDisplay();
    updateDeviceButtons();
    updateMotorControlButtons();
}

void MainWindow::handleDataReceived(const QString& data) {
    addCommunicationLog(data, false);
}

void MainWindow::handleSensorData(const std::string& jsonData) {
    if (jsonData.empty() || jsonData == "{}") {
        return;
    }
    updateSensorMonitorDisplay();
}

void MainWindow::handleMotorStatusChanged(int status) {
    updateMotorControlDisplay();
    updateMotorControlButtons();
}

void MainWindow::handleError(const QString& error) {
    logUserOperation(QString("Error: %1").arg(error));
    showStatusMessage(error, 5000);
}

void MainWindow::setupRawSerialCommunication() {
    if (!m_controller) return;
    
    m_controller->setDataCallback([this](const std::string& data) {
        QMetaObject::invokeMethod(this, [this, data]() {
            QString rxData = QString::fromStdString(data);
            addCommunicationLog(rxData, false);
        }, Qt::QueuedConnection);
    });
}

void MainWindow::updateDataTable() {
}

void MainWindow::onSensorUpdateTimer() {
}

double MainWindow::parseJsonValue(const QJsonValue& value) {
    if (value.isString()) {
        QString str = value.toString();
        if (str == "NaN") {
            return std::numeric_limits<double>::quiet_NaN();
        } else if (str == "Inf" || str == "+Inf") {
            return std::numeric_limits<double>::infinity();
        } else if (str == "-Inf") {
            return -std::numeric_limits<double>::infinity();
        }
        bool ok;
        double d = str.toDouble(&ok);
        return ok ? d : 0.0;
    }
    return value.toDouble();
}

