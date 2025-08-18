// ui/src/mainwindow.cpp
#include "../include/mainwindow.h"
#include "ui_mainwindow.h"

// 项目头文件
#include "../../src/models/include/device_info.h"
#include "../../src/hardware/include/serial_interface.h"
#include "../../src/utils/include/logger.h"
#include "../../src/models/include/system_config.h"
#include "../include/adddevicedialog.h"
#include "../include/confirmdialog.h"
#include "../include/errordialog.h"

// Qt头文件
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <QScrollBar>
#include <QDebug>          // 添加这行
#include <QMessageBox>     // 如果需要的话
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentSelectedDeviceIndex(-1)
    , serialInterface(std::make_shared<SerialInterface>())
    , isInitialized(false)
    , motorController(nullptr)
    , safetyManager(std::make_shared<SafetyManager>())
    , targetHeight(0.0)
    , targetAngle(0.0)
    , currentHeight(0.0)
    , currentAngle(0.0)
    , theoreticalCapacitance(0.0)
    // 添加第三页相关成员变量初始化
    , sensorManager(nullptr)
    , dataRecorder(std::make_shared<DataRecorder>())
    , exportManager(std::make_shared<ExportManager>())
    , currentSensorData()
    , hasValidSensorData(false)
    , lastRecordTime()
    // 添加第四页相关成员变量初始化
    , logUpdateTimer(new QTimer(this))
    , currentLogLevel(LogLevel::ALL)
    , lastDisplayedLogCount(0)
{
    qDebug() << "=== MainWindow构造函数开始 ===";
    qDebug() << "ui指针初始化完成:" << ui;

    ui->setupUi(this);
    setupResponsiveLayout();  // <-- 添加这行

    qDebug() << "setupUi调用完成";

    qDebug() << "检查doubleSpinBox_3:" << ui->doubleSpinBox_3;
    if (!ui->doubleSpinBox_3) {
        qCritical() << "警告：setupUi后doubleSpinBox_3仍为空！";
    }
    
    // 设置窗口属性
    setWindowTitle("CDC_Control_Programm v1.0");
    setMinimumSize(800, 600);
    
    // 初始化第一页（设备管理）
    initializeDeviceManagement();

    // 初始化第二页（电机控制）
    initializeMotorControl();
    initializeSensorMonitor(); // 添加第三页初始化
    initializeLogViewer(); // 添加第四页初始化
    // // 初始化第五页（数据可视化）
    // initializeDataVisualization();
    
    // 设置初始状态
    isInitialized = true;
    
    // 记录启动
    logUserOperation("Application started");
    showStatusMessage("CDC控制系统已启动");
}

MainWindow::~MainWindow()
{
    // 断开所有设备连接
    if (serialInterface && serialInterface->isOpen()) {
        serialInterface->close();
    }
    
    logUserOperation("Application closed");
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 检查是否有设备连接
    bool hasConnectedDevices = false;
    for (const auto& device : deviceList) {
        if (device.isConnected()) {
            hasConnectedDevices = true;
            break;
        }
    }
    
    if (hasConnectedDevices) {
        if (ConfirmDialog::confirm(this, 
                                  "仍有设备连接中，确定要退出吗？",
                                  "确认退出",
                                  "退出",
                                  "取消")) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }

        // 强制更新日志显示
    if (event->isAccepted()) {
        forceLogUpdate();
    }
}

// ===== 第一页：设备管理UI初始化 =====

void MainWindow::initializeDeviceManagement()
{
    // 设置默认标签页为设备管理
    ui->tabWidget->setCurrentIndex(0);
    
    // 连接设备管理相关信号槽
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::onAddDeviceClicked);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::onRemoveDeviceClicked);
    connect(ui->pushButton_20, &QPushButton::clicked, this, &MainWindow::onConnectDeviceClicked);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onDisconnectDeviceClicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::onSendCommandClicked);
    
    // 连接设备列表选择信号
    connect(ui->listWidget, &QListWidget::currentItemChanged, 
            this, &MainWindow::onDeviceSelectionChanged);
    
    // 连接命令输入框回车信号
    connect(ui->lineEdit, &QLineEdit::returnPressed, 
            this, &MainWindow::onCommandLineReturnPressed);
    
    // 设置串口接口回调
    serialInterface->setConnectionCallback([this](bool connected) {
        // 使用Qt的信号槽机制确保在主线程中调用
        QMetaObject::invokeMethod(this, "onSerialConnectionChanged", 
                                 Qt::QueuedConnection, Q_ARG(bool, connected));
    });
    
    serialInterface->setDataReceivedCallback([this](const std::string& data) {
        QString qdata = QString::fromStdString(data);
        QMetaObject::invokeMethod(this, "onSerialDataReceived", 
                                 Qt::QueuedConnection, Q_ARG(QString, qdata));
    });
    
    serialInterface->setErrorCallback([this](const std::string& error) {
        QString qerror = QString::fromStdString(error);
        QMetaObject::invokeMethod(this, "onSerialError", 
                                 Qt::QueuedConnection, Q_ARG(QString, qerror));
    });
    
    // 初始化UI状态
    updateDeviceListWidget();
    updateSelectedDeviceDisplay();
    updateDeviceButtons();
    
    // 清空通信日志
    clearCommunicationLog();
    
    logUserOperation("Device management initialized");
}

// ===== 设备管理槽函数实现 =====

void MainWindow::onAddDeviceClicked()
{
    QStringList existingNames = getExistingDeviceNames();
    QList<DeviceInfo> connectedDevices = getConnectedDevices();
    
    AddDeviceDialog dialog(this, existingNames, connectedDevices);
    
    if (dialog.exec() == QDialog::Accepted) {
        DeviceInfo newDevice = dialog.getDeviceInfo();
        addDeviceToList(newDevice);
        
        logUserOperation(QString("Device added: %1 on %2")
                        .arg(QString::fromStdString(newDevice.getName()))
                        .arg(QString::fromStdString(newDevice.getPortName())));
        
        showStatusMessage(QString("设备 '%1' 已添加")
                         .arg(QString::fromStdString(newDevice.getName())));
    }
}

void MainWindow::onRemoveDeviceClicked()
{
    int index = getCurrentSelectedDeviceIndex();
    if (index < 0 || index >= deviceList.size()) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, "请先选择要删除的设备");
        return;
    }
    
    DeviceInfo& device = deviceList[index];
    QString deviceName = QString::fromStdString(device.getName());
    
    // 如果设备已连接，先断开
    if (device.isConnected()) {
        if (!ConfirmDialog::confirmDisconnectDevice(this, deviceName)) {
            return;
        }
        // 断开连接
        if (serialInterface->isOpen()) {
            serialInterface->close();
        }
        device.setConnectionStatus(ConnectionStatus::DISCONNECTED);
    }
    
    // 确认删除
    if (ConfirmDialog::confirmDeleteDevice(this, deviceName)) {
        removeDeviceFromList(index);
        
        logUserOperation(QString("Device removed: %1").arg(deviceName));
        showStatusMessage(QString("设备 '%1' 已删除").arg(deviceName));
    }
}

void MainWindow::onConnectDeviceClicked()
{
    DeviceInfo* device = getCurrentSelectedDevice();
    if (!device) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, "请先选择要连接的设备");
        return;
    }
    
    if (device->isConnected()) {
        showStatusMessage("设备已经连接");
        return;
    }
    
    // 如果有其他设备连接，先断开
    if (serialInterface->isOpen()) {
        serialInterface->close();
        // 更新其他设备状态
        for (auto& dev : deviceList) {
            if (dev.isConnected()) {
                dev.setConnectionStatus(ConnectionStatus::DISCONNECTED);
            }
        }
    }
    
    // 尝试连接
    device->setConnectionStatus(ConnectionStatus::CONNECTING);
    updateDeviceListWidget();
    updateSelectedDeviceDisplay();
    updateDeviceButtons();
    
    std::string portName = device->getPortName();
    int baudRate = device->getBaudRate();
    
    if (serialInterface->open(portName, baudRate)) {
        device->setConnectionStatus(ConnectionStatus::CONNECTED);
        device->updateLastActivityTime();
        
        logUserOperation(QString("Device connected: %1 on %2 @ %3")
                        .arg(QString::fromStdString(device->getName()))
                        .arg(QString::fromStdString(portName))
                        .arg(baudRate));
        
        showStatusMessage(QString("设备 '%1' 连接成功")
                         .arg(QString::fromStdString(device->getName())));
        
        addCommunicationLog(QString("=== 连接到设备: %1 ===")
                           .arg(QString::fromStdString(device->getName())));
    } else {
        device->setConnectionStatus(ConnectionStatus::ERROR);
        device->recordError("Connection failed");
        
        ErrorDialog::showError(this, ErrorDialog::CommunicationError, 
                             QString("无法连接到设备 '%1'")
                             .arg(QString::fromStdString(device->getName())));
        
        logUserOperation(QString("Device connection failed: %1")
                        .arg(QString::fromStdString(device->getName())));
    }
    
    updateDeviceListWidget();
    updateSelectedDeviceDisplay();
    updateDeviceButtons();
}

void MainWindow::onDisconnectDeviceClicked()
{
    DeviceInfo* device = getCurrentSelectedDevice();
    if (!device) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, "请先选择要断开的设备");
        return;
    }
    
    if (!device->isConnected()) {
        showStatusMessage("设备未连接");
        return;
    }
    
    QString deviceName = QString::fromStdString(device->getName());
    
    if (ConfirmDialog::confirmDisconnectDevice(this, deviceName)) {
        if (serialInterface->isOpen()) {
            serialInterface->close();
        }
        
        device->setConnectionStatus(ConnectionStatus::DISCONNECTED);
        
        logUserOperation(QString("Device disconnected: %1").arg(deviceName));
        showStatusMessage(QString("设备 '%1' 已断开连接").arg(deviceName));
        
        addCommunicationLog(QString("=== 断开设备: %1 ===").arg(deviceName));
        
        updateDeviceListWidget();
        updateSelectedDeviceDisplay();
        updateDeviceButtons();
    }
}

void MainWindow::onSendCommandClicked()
{
    QString command = ui->lineEdit->text().trimmed();
    if (command.isEmpty()) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, "请输入要发送的命令");
        return;
    }
    
    sendCommandToCurrentDevice(command);
    ui->lineEdit->clear();
}

void MainWindow::onDeviceSelectionChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    
    if (current) {
        // 从ListWidget的行号获取设备索引
        currentSelectedDeviceIndex = ui->listWidget->row(current);
    } else {
        currentSelectedDeviceIndex = -1;
    }
    
    updateSelectedDeviceDisplay();
    updateDeviceButtons();
}

void MainWindow::onCommandLineReturnPressed()
{
    onSendCommandClicked();
}



void MainWindow::onSerialDataReceived(const QString& data)
{
    addCommunicationLog(data, false); // 接收的数据
    
    DeviceInfo* device = getCurrentSelectedDevice();
    if (device) {
        device->updateLastActivityTime();
    }
}

void MainWindow::onSerialError(const QString& error)
{
    addCommunicationLog(QString("错误: %1").arg(error), false);
    
    DeviceInfo* device = getCurrentSelectedDevice();
    if (device) {
        device->recordError(error.toStdString());
        device->setConnectionStatus(ConnectionStatus::ERROR);
        
        updateDeviceListWidget();
        updateSelectedDeviceDisplay();
        updateDeviceButtons();
    }
    
    ErrorDialog::showError(this, ErrorDialog::CommunicationError, 
                          QString("通信错误: %1").arg(error));
    // 强制更新日志显示
    forceLogUpdate();
}

// ===== 设备管理辅助方法实现 =====

void MainWindow::updateDeviceListWidget()
{
    ui->listWidget->clear();
    
    for (int i = 0; i < deviceList.size(); ++i) {
        const DeviceInfo& device = deviceList[i];
        QString displayText = getDeviceDisplayText(device);
        
        QListWidgetItem* item = new QListWidgetItem(displayText);
        
        // 根据连接状态设置图标和颜色
        switch (device.getConnectionStatus()) {
            case ConnectionStatus::CONNECTED:
                item->setForeground(QBrush(QColor(0, 128, 0))); // 绿色
                break;
            case ConnectionStatus::CONNECTING:
                item->setForeground(QBrush(QColor(255, 165, 0))); // 橙色
                break;
            case ConnectionStatus::ERROR:
                item->setForeground(QBrush(QColor(255, 0, 0))); // 红色
                break;
            case ConnectionStatus::DISCONNECTED:
            default:
                item->setForeground(QBrush(QColor(0, 0, 0))); // 黑色
                break;
        }
        
        ui->listWidget->addItem(item);
    }
    
    // 恢复选择
    if (currentSelectedDeviceIndex >= 0 && currentSelectedDeviceIndex < deviceList.size()) {
        ui->listWidget->setCurrentRow(currentSelectedDeviceIndex);
    }
}

void MainWindow::updateSelectedDeviceDisplay()
{
    DeviceInfo* device = getCurrentSelectedDevice();
    if (device) {
        QString deviceName = QString::fromStdString(device->getName());
        QString statusText = QString::fromStdString(device->getConnectionStatusString());
        ui->label_3->setText(QString("%1 (%2)").arg(deviceName, statusText));
    } else {
        ui->label_3->setText("No device selected");
    }
}

void MainWindow::updateDeviceButtons()
{
    bool hasSelection = getCurrentSelectedDevice() != nullptr;
    DeviceInfo* device = getCurrentSelectedDevice();
    bool isConnected = device && device->isConnected();
    bool isDisconnected = device && !device->isConnected();
    
    // 添加设备按钮始终可用
    ui->pushButton_3->setEnabled(true);
    
    // 删除设备按钮
    ui->pushButton_4->setEnabled(hasSelection);
    
    // 连接按钮
    ui->pushButton_20->setEnabled(hasSelection && isDisconnected);
    
    // 断开连接按钮
    ui->pushButton->setEnabled(hasSelection && isConnected);
    
    // 发送命令相关
    ui->pushButton_2->setEnabled(isConnected);
    ui->lineEdit->setEnabled(isConnected);
}

void MainWindow::addDeviceToList(const DeviceInfo& device)
{
    deviceList.append(device);
    updateDeviceListWidget();
    
    // 选中新添加的设备
    currentSelectedDeviceIndex = deviceList.size() - 1;
    ui->listWidget->setCurrentRow(currentSelectedDeviceIndex);
    
    updateSelectedDeviceDisplay();
    updateDeviceButtons();
}

void MainWindow::removeDeviceFromList(int index)
{
    if (index >= 0 && index < deviceList.size()) {
        deviceList.removeAt(index);
        
        // 调整选中索引
        if (currentSelectedDeviceIndex == index) {
            currentSelectedDeviceIndex = -1;
        } else if (currentSelectedDeviceIndex > index) {
            currentSelectedDeviceIndex--;
        }
        
        updateDeviceListWidget();
        updateSelectedDeviceDisplay();
        updateDeviceButtons();
    }
}

DeviceInfo* MainWindow::getCurrentSelectedDevice()
{
    if (currentSelectedDeviceIndex >= 0 && currentSelectedDeviceIndex < deviceList.size()) {
        return &deviceList[currentSelectedDeviceIndex];
    }
    return nullptr;
}

int MainWindow::getCurrentSelectedDeviceIndex() const
{
    return currentSelectedDeviceIndex;
}

QStringList MainWindow::getExistingDeviceNames() const
{
    QStringList names;
    for (const auto& device : deviceList) {
        names << QString::fromStdString(device.getName());
    }
    return names;
}

QList<DeviceInfo> MainWindow::getConnectedDevices() const
{
    QList<DeviceInfo> connected;
    for (const auto& device : deviceList) {
        if (device.isConnected()) {
            connected << device;
        }
    }
    return connected;
}

void MainWindow::sendCommandToCurrentDevice(const QString& command)
{
    DeviceInfo* device = getCurrentSelectedDevice();
    if (!device || !device->isConnected()) {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError, "没有连接的设备");
        return;
    }
    
    if (!serialInterface->isOpen()) {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError, "串口未打开");
        return;
    }
    
    // 添加命令终止符（如果没有的话）
    QString fullCommand = command;
    if (!fullCommand.endsWith("\r\n")) {
        fullCommand += "\r\n";
    }
    
    if (serialInterface->sendCommand(fullCommand.toStdString())) {
        addCommunicationLog(QString(">> %1").arg(command), true);
        device->updateLastActivityTime();
        
        logUserOperation(QString("Command sent to %1: %2")
                        .arg(QString::fromStdString(device->getName()))
                        .arg(command));
    } else {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError, "命令发送失败");
        addCommunicationLog(QString(">> 发送失败: %1").arg(command), true);
    }
}

void MainWindow::addCommunicationLog(const QString& message, bool isOutgoing)
{
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("hh:mm:ss.zzz");
    
    QString prefix = isOutgoing ? "TX" : "RX";
    QString logLine = QString("[%1] %2: %3").arg(timestamp, prefix, message);
    
    ui->textEdit->append(logLine);
    
    // 限制日志行数
    if (ui->textEdit->document()->lineCount() > MAX_COMMUNICATION_LOG_LINES) {
        QTextCursor cursor = ui->textEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 100);
        cursor.removeSelectedText();
    }
    
    // 自动滚动到底部
    QScrollBar* scrollBar = ui->textEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::clearCommunicationLog()
{
    ui->textEdit->clear();
    addCommunicationLog("=== 通信日志已清空 ===");
}

QString MainWindow::getDeviceDisplayText(const DeviceInfo& device) const
{
    QString name = QString::fromStdString(device.getName());
    QString port = QString::fromStdString(device.getPortName());
    QString status = QString::fromStdString(device.getConnectionStatusString());
    
    return QString("%1 [%2] - %3").arg(name, port, status);
}

// ===== 通用辅助方法实现 =====

void MainWindow::showStatusMessage(const QString& message, int timeout)
{
    if (ui->statusbar) {
        ui->statusbar->showMessage(message, timeout);
    }
}

bool MainWindow::isDeviceNameExists(const QString& name) const
{
    for (const auto& device : deviceList) {
        if (QString::fromStdString(device.getName()) == name) {
            return true;
        }
    }
    return false;
}

bool MainWindow::isPortInUse(const QString& portName) const
{
    for (const auto& device : deviceList) {
        if (device.isConnected() && 
            QString::fromStdString(device.getPortName()) == portName) {
            return true;
        }
    }
    return false;
}

// ===== 第二页：电机控制UI初始化 =====

void MainWindow::initializeMotorControl()
{
    // 创建电机控制器（延迟创建，等串口连接后）
    motorController = nullptr;
    
    // 连接UI信号槽
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
    
    // 初始化UI状态
    setupMotorControlUI();
    updateMotorControlDisplay();
    updateMotorControlButtons();
    
    logUserOperation("Motor control initialized");
}

void MainWindow::setupMotorControlUI()
{

    qDebug() << "=== setupMotorControlUI开始 ===";
    qDebug() << "this指针:" << this;
    qDebug() << "ui指针:" << ui;
    
    if (!ui) {
        qCritical() << "致命错误：ui指针为空！";
        return;
    }
    
    qDebug() << "检查doubleSpinBox_3:" << ui->doubleSpinBox_3;
    
    if (!ui->doubleSpinBox_3) {
        qCritical() << "doubleSpinBox_3 为空指针！";
        return;
    }

    // 从系统配置加载安全限位
    auto& config = SystemConfig::getInstance();

    qDebug() << "准备设置doubleSpinBox_3的值:" << config.getMaxHeight();
    
    // 设置安全限位SpinBox的默认值
    ui->doubleSpinBox_3->setValue(config.getMaxHeight());  // 最大高度
    ui->doubleSpinBox_8->setValue(config.getMinHeight());  // 最小高度
    ui->doubleSpinBox_6->setValue(config.getMinAngle());   // 最小角度
    ui->doubleSpinBox_7->setValue(config.getMaxAngle());   // 最大角度
    
    // 设置目标位置SpinBox的范围和默认值
    ui->doubleSpinBox->setRange(config.getMinHeight(), config.getMaxHeight());
    ui->doubleSpinBox->setValue(0.0);
    
    ui->doubleSpinBox_2->setRange(config.getMinAngle(), config.getMaxAngle());
    ui->doubleSpinBox_2->setValue(0.0);
    
    // 设置初始显示值
    targetHeight = 0.0;
    targetAngle = 0.0;
    currentHeight = 0.0;
    currentAngle = 0.0;
    theoreticalCapacitance = 0.0;
    
    // 更新安全管理器的限位
    updateSafetyManagerLimits();
}

// ===== 第二页：电机控制槽函数实现 =====

void MainWindow::onSetHeightClicked()
{
    double height = ui->doubleSpinBox->value();
    
    // 安全检查
    if (!checkSafetyLimits(height, targetAngle)) {
        return; // 错误对话框已在checkSafetyLimits中显示
    }
    
    targetHeight = height;
    
    logUserOperation(QString("Target height set to %.1f mm").arg(height));
    showStatusMessage(QString("目标高度设置为 %.1f mm").arg(height));
}

void MainWindow::onSetAngleClicked()
{
    double angle = ui->doubleSpinBox_2->value();
    
    // 安全检查
    if (!checkSafetyLimits(targetHeight, angle)) {
        return; // 错误对话框已在checkSafetyLimits中显示
    }
    
    targetAngle = angle;
    
    logUserOperation(QString("Target angle set to %.1f degrees").arg(angle));
    showStatusMessage(QString("目标角度设置为 %.1f°").arg(angle));
}

void MainWindow::onMoveToPositionClicked()
{
    // 检查设备连接
    DeviceInfo* device = getCurrentSelectedDevice();
    if (!device || !device->isConnected()) {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError, "请先连接设备");
        return;
    }
    
    // 安全检查
    if (!checkSafetyLimits(targetHeight, targetAngle)) {
        return;
    }
    
    // 创建电机控制器（如果还没有）
    if (!motorController) {
        createMotorController();
    }
    
    if (!motorController) {
        ErrorDialog::showError(this, ErrorDialog::HardwareError, "电机控制器初始化失败");
        return;
    }
    
    // 发送移动命令
    bool success = motorController->moveToPosition(targetHeight, targetAngle);
    
    if (success) {
        // 更新当前位置显示（发送成功后立即更新）
        currentHeight = targetHeight;
        currentAngle = targetAngle;
        
        // 计算并更新理论电容
        updateTheoreticalCapacitance();
        
        // 更新显示
        updateMotorControlDisplay();
        
        logUserOperation(QString("Move command sent: height=%.1f mm, angle=%.1f°")
                        .arg(targetHeight).arg(targetAngle));
        showStatusMessage(QString("移动命令已发送: %.1f mm, %.1f°")
                         .arg(targetHeight).arg(targetAngle));
    } else {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError, "移动命令发送失败");
    }
}

void MainWindow::onHomePositionClicked()
{
    // 检查设备连接
    DeviceInfo* device = getCurrentSelectedDevice();
    if (!device || !device->isConnected()) {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError, "请先连接设备");
        return;
    }
    
    if (!motorController) {
        createMotorController();
    }
    
    if (!motorController) {
        ErrorDialog::showError(this, ErrorDialog::HardwareError, "电机控制器初始化失败");
        return;
    }
    
    bool success = motorController->home();
    
    if (success) {
        // 更新到原点位置
        auto& config = SystemConfig::getInstance();
        currentHeight = config.getHomeHeight();
        currentAngle = config.getHomeAngle();
        targetHeight = currentHeight;
        targetAngle = currentAngle;
        
        // 更新UI
        ui->doubleSpinBox->setValue(targetHeight);
        ui->doubleSpinBox_2->setValue(targetAngle);
        
        // 更新理论电容和显示
        updateTheoreticalCapacitance();
        updateMotorControlDisplay();
        
        logUserOperation("Home position command sent");
        showStatusMessage("回零命令已发送");
    } else {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError, "回零命令发送失败");
    }
}

void MainWindow::onStopMotorClicked()
{
    if (!motorController) {
        showStatusMessage("电机控制器未初始化");
        return;
    }
    
    bool success = motorController->stop();
    
    if (success) {
        logUserOperation("Motor stop command sent");
        showStatusMessage("停止命令已发送");
    } else {
        ErrorDialog::showError(this, ErrorDialog::CommunicationError, "停止命令发送失败");
    }
}

void MainWindow::onEmergencyStopClicked()
{
    // 紧急停止不检查连接状态，直接发送
    if (motorController) {
        motorController->emergencyStop();
    }
    
    // 也通过串口直接发送紧急停止命令
    if (serialInterface && serialInterface->isOpen()) {
        serialInterface->sendCommand("EMERGENCY_STOP\r\n");
    }
    
    // 触发安全管理器的紧急停止
    safetyManager->triggerEmergencyStop("User activated emergency stop");
    
    logUserOperation("EMERGENCY STOP activated");
    showStatusMessage("紧急停止已激活", 10000); // 显示10秒
    
    // 更新UI状态
    updateMotorControlButtons();
}

void MainWindow::onSafetyLimitsChanged()
{
    updateSafetyManagerLimits();
    updateTargetPositionRanges();
    
    logUserOperation("Safety limits updated");
    showStatusMessage("安全限位已更新");
}

// ===== 第二页：辅助方法实现 =====

void MainWindow::updateMotorControlDisplay()
{
    // 更新当前位置显示
    ui->label_11->setText(QString("%.1f mm").arg(currentHeight));
    ui->label_8->setText(QString("%.1f°").arg(currentAngle));
    
    // 更新理论电容显示
    ui->label_13->setText(QString("%.1f pF").arg(theoreticalCapacitance));
    
    // 更新电机状态
    QString motorStatus = "Ready";
    if (motorController) {
        switch (motorController->getStatus()) {
            case MotorStatus::IDLE:
                motorStatus = "Ready";
                break;
            case MotorStatus::MOVING:
                motorStatus = "Moving";
                break;
            case MotorStatus::ERROR:
                motorStatus = "Error";
                break;
            case MotorStatus::HOMING:
                motorStatus = "Homing";
                break;
            case MotorStatus::CALIBRATING:
                motorStatus = "Calibrating";
                break;
        }
    }
    ui->label_19->setText(QString("Motor Status: %1").arg(motorStatus));
    
    // 更新连接状态
    QString connectionStatus = "Disconnected";
    DeviceInfo* device = getCurrentSelectedDevice();
    if (device && device->isConnected()) {
        connectionStatus = "Connected";
    }
    ui->label_20->setText(QString("Connection Status: %1").arg(connectionStatus));
}

void MainWindow::updateMotorControlButtons()
{
    DeviceInfo* device = getCurrentSelectedDevice();
    bool isConnected = device && device->isConnected();
    bool isEmergencyStopped = safetyManager->isEmergencyStopped();
    bool isMotorReady = motorController && 
                       (motorController->getStatus() == MotorStatus::IDLE);
    
    // Set Height/Angle 按钮始终可用（但会在点击时检查安全限位）
    ui->pushButton_5->setEnabled(true);  // Set Height
    ui->pushButton_9->setEnabled(true);  // Set Angle
    
    // Move to Position 需要连接且非紧急停止状态
    ui->pushButton_7->setEnabled(isConnected && !isEmergencyStopped);
    
    // Home Position 需要连接且非紧急停止状态
    ui->pushButton_8->setEnabled(isConnected && !isEmergencyStopped);
    
    // Stop Motor 需要连接且电机在运行
    bool isMoving = motorController && 
                   (motorController->getStatus() == MotorStatus::MOVING ||
                    motorController->getStatus() == MotorStatus::HOMING);
    ui->pushButton_6->setEnabled(isConnected && isMoving);
    
    // Emergency Stop 始终可用
    ui->pushButton_12->setEnabled(true);
    
    // 安全限位设置始终可用
    ui->doubleSpinBox_3->setEnabled(true);
    ui->doubleSpinBox_8->setEnabled(true);
    ui->doubleSpinBox_6->setEnabled(true);
    ui->doubleSpinBox_7->setEnabled(true);
    
    // 目标位置设置在紧急停止时禁用
    ui->doubleSpinBox->setEnabled(!isEmergencyStopped);
    ui->doubleSpinBox_2->setEnabled(!isEmergencyStopped);
}

bool MainWindow::checkSafetyLimits(double height, double angle)
{
    if (!safetyManager->checkPosition(height, angle)) {
        QString message = QString("位置超出安全限位!\n")
                         + QString("高度: %.1f mm (限位: %.1f - %.1f mm)\n")
                           .arg(height)
                           .arg(ui->doubleSpinBox_8->value())
                           .arg(ui->doubleSpinBox_3->value())
                         + QString("角度: %.1f° (限位: %.1f° - %.1f°)")
                           .arg(angle)
                           .arg(ui->doubleSpinBox_6->value())
                           .arg(ui->doubleSpinBox_7->value());
        
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, message);
        return false;
    }
    
    return true;
}

void MainWindow::updateSafetyManagerLimits()
{
    double maxHeight = ui->doubleSpinBox_3->value();
    double minHeight = ui->doubleSpinBox_8->value();
    double minAngle = ui->doubleSpinBox_6->value();
    double maxAngle = ui->doubleSpinBox_7->value();
    
    // 验证限位设置的合理性
    if (minHeight >= maxHeight) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, 
                              "最小高度不能大于等于最大高度");
        return;
    }
    
    if (minAngle >= maxAngle) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, 
                              "最小角度不能大于等于最大角度");
        return;
    }
    
    // 更新安全管理器
    safetyManager->setCustomLimits(minHeight, maxHeight, minAngle, maxAngle);
    
    // 更新系统配置
    auto& config = SystemConfig::getInstance();
    config.setHeightLimits(minHeight, maxHeight);
    config.setAngleLimits(minAngle, maxAngle);
}

void MainWindow::updateTargetPositionRanges()
{
    double maxHeight = ui->doubleSpinBox_3->value();
    double minHeight = ui->doubleSpinBox_8->value();
    double minAngle = ui->doubleSpinBox_6->value();
    double maxAngle = ui->doubleSpinBox_7->value();
    
    // 更新目标位置SpinBox的范围
    ui->doubleSpinBox->setRange(minHeight, maxHeight);
    ui->doubleSpinBox_2->setRange(minAngle, maxAngle);
}

void MainWindow::updateTheoreticalCapacitance()
{
    // 使用MathUtils计算理论电容
    auto& config = SystemConfig::getInstance();
    double plateArea = config.getPlateArea();  // 如果SystemConfig中没有，使用默认值
    double dielectric = config.getDielectricConstant();
    
    // 使用当前高度作为板间距离
    theoreticalCapacitance = MathUtils::calculateParallelPlateCapacitance(
        plateArea, currentHeight, currentAngle, dielectric);
}

void MainWindow::createMotorController()
{
    if (!motorController && serialInterface) {
        motorController = std::make_shared<MotorController>(serialInterface, safetyManager);
        
        // 设置回调
        motorController->setStatusCallback([this](MotorStatus status) {
            QMetaObject::invokeMethod(this, "onMotorStatusChanged", 
                                     Qt::QueuedConnection, 
                                     Q_ARG(int, static_cast<int>(status)));
        });
        
        motorController->setErrorCallback([this](const MotorError& error) {
            QString errorMsg = QString::fromStdString(error.message);
            QMetaObject::invokeMethod(this, "onMotorError", 
                                     Qt::QueuedConnection, 
                                     Q_ARG(QString, errorMsg));
        });
    }
}

// ===== 电机控制器回调处理 =====

void MainWindow::onMotorStatusChanged(int status)
{
    updateMotorControlDisplay();
    updateMotorControlButtons();
    
    MotorStatus motorStatus = static_cast<MotorStatus>(status);
    QString statusStr;
    
    switch (motorStatus) {
        case MotorStatus::IDLE:
            statusStr = "就绪";
            break;
        case MotorStatus::MOVING:
            statusStr = "移动中";
            break;
        case MotorStatus::ERROR:
            statusStr = "错误";
            break;
        case MotorStatus::HOMING:
            statusStr = "回零中";
            break;
        case MotorStatus::CALIBRATING:
            statusStr = "校准中";
            break;
    }
    
    showStatusMessage(QString("电机状态: %1").arg(statusStr));
}

void MainWindow::onMotorError(const QString& error)
{
    ErrorDialog::showError(this, ErrorDialog::HardwareError, 
                          QString("电机错误: %1").arg(error));
    
    updateMotorControlDisplay();
    updateMotorControlButtons();

    // 强制更新日志显示
    forceLogUpdate();
}

// ===== 第三页：传感器监控UI初始化 =====

void MainWindow::initializeSensorMonitor()
{
    // 创建传感器管理器（延迟创建，等串口连接后）
    sensorManager = nullptr;
    
    // 连接UI信号槽
    connect(ui->pushButton_13, &QPushButton::clicked, this, &MainWindow::onRecordDataClicked);
    connect(ui->pushButton_14, &QPushButton::clicked, this, &MainWindow::onExportDataClicked);
    
    // 设置导出管理器回调
    exportManager->setProgressCallback([this](int percentage) {
        // 可以在状态栏显示进度
        QMetaObject::invokeMethod(this, "onExportProgress", 
                                 Qt::QueuedConnection, Q_ARG(int, percentage));
    });
    
    exportManager->setCompletionCallback([this](const ExportStatistics& stats) {
        QString message = QString("导出完成: %1 条记录").arg(stats.exportedRecords);
        QMetaObject::invokeMethod(this, "showStatusMessage", 
                                 Qt::QueuedConnection, Q_ARG(QString, message));
    });
    
    // 设置数据记录器回调
    dataRecorder->setDataChangeCallback([this](int recordCount) {
        // 更新UI显示记录数量
        QMetaObject::invokeMethod(this, "onRecordCountChanged", 
                                 Qt::QueuedConnection, Q_ARG(int, recordCount));
    });
    
    // 初始化UI状态
    updateSensorMonitorDisplay();
    
    logUserOperation("Sensor monitor initialized");
}

void MainWindow::createSensorManager()
{
    if (!sensorManager && serialInterface) {
        sensorManager = std::make_shared<SensorManager>(serialInterface);
        
        // 设置回调
        sensorManager->setDataCallback([this](const SensorData& data) {
            QMetaObject::invokeMethod(this, "onSensorDataReceived", 
                                     Qt::QueuedConnection, 
                                     Q_ARG(SensorData, data));
        });
        
        sensorManager->setErrorCallback([this](const std::string& error) {
            QString errorMsg = QString::fromStdString(error);
            QMetaObject::invokeMethod(this, "onSensorError", 
                                     Qt::QueuedConnection, 
                                     Q_ARG(QString, errorMsg));
        });
        
        // 从系统配置获取更新间隔
        auto& config = SystemConfig::getInstance();
        sensorManager->setUpdateInterval(config.getSensorUpdateInterval());
        
        LOG_INFO("SensorManager created");
    }
}

// ===== 第三页：传感器监控槽函数实现 =====

void MainWindow::onRecordDataClicked()
{
    // 检查是否有有效的传感器数据
    if (!hasValidSensorData) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, 
                              "当前没有有效的传感器数据，请稍后再试");
        return;
    }
    
    // 获取第二页的设定值（如果第二页已经设置过）
    double setHeight = currentHeight;  // 第二页最后发送的高度
    double setAngle = currentAngle;    // 第二页最后发送的角度
    
    try {
        // 记录当前状态
        dataRecorder->recordCurrentState(setHeight, setAngle, currentSensorData);
        
        // 更新最后记录时间显示
        lastRecordTime = QDateTime::currentDateTime();
        ui->label_38->setText(lastRecordTime.toString("hh:mm:ss"));
        
        logUserOperation(QString("Data recorded: Height=%.1fmm, Angle=%.1f°, Records=%1")
                        .arg(setHeight).arg(setAngle).arg(dataRecorder->getRecordCount()));
        
        showStatusMessage(QString("数据已记录 (共%1条)").arg(dataRecorder->getRecordCount()));
        
    } catch (const std::exception& e) {
        ErrorDialog::showError(this, ErrorDialog::HardwareError, 
                              QString("记录数据失败: %1").arg(e.what()));
    }
}

void MainWindow::onExportDataClicked()
{
    if (!dataRecorder->hasData()) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, "没有数据可导出");
        return;
    }
    
    // 使用系统文件对话框选择保存位置
    QString defaultFilename = QString::fromStdString(dataRecorder->getDefaultFilename());
    QString filename = QFileDialog::getSaveFileName(
        this,
        "导出测量数据",
        defaultFilename,
        "CSV文件 (*.csv);;所有文件 (*.*)"
    );
    
    if (filename.isEmpty()) {
        return; // 用户取消
    }
    
    try {
        // 获取所有测量数据
        auto measurements = dataRecorder->getAllMeasurements();
        
        // 设置导出选项
        ExportOptions options;
        options.format = ExportFormat::CSV;
        options.includeTimestamp = true;
        options.includeSetValues = true;
        options.includeSensorData = true;
        options.includeCalculatedValues = true;
        options.decimalPlaces = 2;
        
        // 执行导出
        bool success = exportManager->exportData(measurements, filename.toStdString(), options);
        
        if (success) {
            auto stats = exportManager->getLastExportStatistics();
            QString message = QString("成功导出 %1 条记录到文件\n文件大小: %2 KB")
                             .arg(stats.exportedRecords)
                             .arg(stats.fileSize / 1024);
            
            QMessageBox::information(this, "导出成功", message);
            
            logUserOperation(QString("Data exported: %1 records to %2")
                            .arg(stats.exportedRecords)
                            .arg(QString::fromStdString(filename.toStdString())));
        } else {
            QString error = QString::fromStdString(exportManager->getLastError());
            ErrorDialog::showError(this, ErrorDialog::FileOperationError, 
                                  QString("导出失败: %1").arg(error));
        }
        
    } catch (const std::exception& e) {
        ErrorDialog::showError(this, ErrorDialog::HardwareError, 
                              QString("导出过程中发生错误: %1").arg(e.what()));
    }
}

void MainWindow::onSensorDataReceived(const SensorData& data)
{
    // 更新当前传感器数据
    currentSensorData = data;
    hasValidSensorData = data.isAllValid();
    
    // 更新UI显示
    updateSensorMonitorDisplay();
    
    // 在日志中记录（仅用于调试，可以设置日志级别控制）
    if (data.isAllValid()) {
        LOG_INFO_F("Sensor data updated: Upper[%.1f,%.1f] Lower[%.1f,%.1f] Temp:%.1f°C",
                   data.distanceUpper1, data.distanceUpper2,
                   data.distanceLower1, data.distanceLower2,
                   data.temperature);
    }
}

void MainWindow::onSensorError(const QString& error)
{
    // 显示传感器错误，但不弹出对话框（避免干扰）
    showStatusMessage(QString("传感器错误: %1").arg(error), 5000);
    
    // 标记数据无效
    hasValidSensorData = false;
    
    // 可以选择显示错误状态
    updateSensorMonitorDisplay();

    // 强制更新日志显示
    forceLogUpdate();
}

void MainWindow::onRecordCountChanged(int recordCount)
{
    // 可以在界面上显示当前记录数量（如果需要的话）
    QString status = QString("已记录 %1 条数据").arg(recordCount);
    showStatusMessage(status, 2000);
}

void MainWindow::onExportProgress(int percentage)
{
    // 在状态栏显示导出进度
    showStatusMessage(QString("导出进度: %1%").arg(percentage), 100);
}

// ===== 第三页：辅助方法实现 =====

void MainWindow::updateSensorMonitorDisplay()
{
    if (!hasValidSensorData) {
        // 显示无效数据状态
        ui->label_23->setText("--");   // Board Distance
        ui->label_25->setText("--");   // Board Angle  
        ui->label_27->setText("--");   // Temperature
        ui->label_30->setText("--");   // Measured Capacitance
        ui->label_32->setText("--");   // Theoretical Capacitance
        ui->label_34->setText("--");   // Difference
        
        // 详细传感器数据
        ui->label_44->setText("--");   // Upper Corner1
        ui->label_45->setText("--");   // Upper Corner2
        ui->label_65->setText("--");   // Average Height
        ui->label_62->setText("--");   // Calculated Angle
        ui->label_53->setText("--");   // Lower Corner1
        ui->label_54->setText("--");   // Lower Corner2
        ui->label_56->setText("--");   // Average Ground Distance
        ui->label_64->setText("--");   // Calculated Upper Height
        
        return;
    }
    
    // 更新Primary Measurements
    double boardDistance = currentSensorData.getAverageHeight();
    double boardAngle = currentSensorData.angle;
    double temperature = currentSensorData.temperature;
    
    ui->label_23->setText(QString("%.1f mm").arg(boardDistance));
    ui->label_25->setText(QString("%.1f°").arg(boardAngle));
    ui->label_27->setText(QString("%.1f°C").arg(temperature));
    
    // 更新Capacitance Data
    double measuredCap = currentSensorData.capacitance;
    double theoreticalCap = calculateTheoreticalCapacitance();
    double difference = measuredCap - theoreticalCap;
    
    ui->label_30->setText(QString("%.1f pF").arg(measuredCap));
    ui->label_32->setText(QString("%.1f pF").arg(theoreticalCap));
    ui->label_34->setText(QString("%.1f pF").arg(difference));
    
    // 更新Detailed Sensor Data
    ui->label_44->setText(QString("%.0f mm").arg(currentSensorData.distanceUpper1));
    ui->label_45->setText(QString("%.0f mm").arg(currentSensorData.distanceUpper2));
    ui->label_65->setText(QString("%.0f mm").arg(currentSensorData.getAverageHeight()));
    ui->label_62->setText(QString("%.1f°").arg(currentSensorData.getCalculatedAngle()));
    
    ui->label_53->setText(QString("%.0f mm").arg(currentSensorData.distanceLower1));
    ui->label_54->setText(QString("%.0f mm").arg(currentSensorData.distanceLower2));
    ui->label_56->setText(QString("%.0f mm").arg(currentSensorData.getAverageGroundDistance()));
    ui->label_64->setText(QString("%.0f mm").arg(currentSensorData.getCalculatedUpperDistance()));
}

double MainWindow::calculateTheoreticalCapacitance() const
{
    if (!hasValidSensorData) {
        return 0.0;
    }
    
    // 使用MathUtils计算理论电容
    auto& config = SystemConfig::getInstance();
    double plateArea = config.getPlateArea();
    double dielectric = config.getDielectricConstant();
    
    // 使用传感器测得的平均高度和角度
    double distance = currentSensorData.getAverageHeight();
    double angle = currentSensorData.getCalculatedAngle();
    
    return MathUtils::calculateParallelPlateCapacitance(plateArea, distance, angle, dielectric);
}

void MainWindow::startSensorMonitoring()
{
    // 检查设备连接
    DeviceInfo* device = getCurrentSelectedDevice();
    if (!device || !device->isConnected()) {
        return;
    }
    
    // 创建传感器管理器（如果还没有）
    if (!sensorManager) {
        createSensorManager();
    }
    
    if (sensorManager && !sensorManager->isRunning()) {
        if (sensorManager->start()) {
            logUserOperation("Sensor monitoring started");
            showStatusMessage("传感器监控已启动");
        } else {
            ErrorDialog::showError(this, ErrorDialog::CommunicationError, "无法启动传感器监控");
        }
    }
}

void MainWindow::stopSensorMonitoring()
{
    if (sensorManager && sensorManager->isRunning()) {
        sensorManager->stop();
        logUserOperation("Sensor monitoring stopped");
        showStatusMessage("传感器监控已停止");
    }
}

// ===== 第四页：日志查看UI初始化 =====

void MainWindow::initializeLogViewer()
{
    // 连接UI信号槽
    connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLogLevelChanged);
    connect(ui->pushButton_15, &QPushButton::clicked, this, &MainWindow::onClearLogClicked);
    connect(ui->pushButton_10, &QPushButton::clicked, this, &MainWindow::onSaveLogClicked);
    
    // 设置日志级别下拉框
    ui->comboBox_2->clear();
    ui->comboBox_2->addItem("All", static_cast<int>(LogLevel::ALL));
    ui->comboBox_2->addItem("Info", static_cast<int>(LogLevel::INFO));
    ui->comboBox_2->addItem("Warning", static_cast<int>(LogLevel::WARNING));
    ui->comboBox_2->addItem("Error", static_cast<int>(LogLevel::ERROR));
    ui->comboBox_2->setCurrentIndex(0); // 默认选择All
    
    // 设置日志显示区域属性
    ui->textEdit_2->setReadOnly(true);
    ui->textEdit_2->document()->setMaximumBlockCount(MAX_LOG_DISPLAY_LINES); // 限制最大行数
    
    // 设置字体（等宽字体便于阅读）
    QFont logFont("Consolas, Monaco, monospace");
    logFont.setPointSize(9);
    ui->textEdit_2->setFont(logFont);
    
    // 连接日志更新定时器
    connect(logUpdateTimer, &QTimer::timeout, this, &MainWindow::updateLogDisplay);
    logUpdateTimer->start(500); // 每500ms检查一次新日志
    
    // 初始化显示
    currentLogLevel = LogLevel::ALL;
    lastDisplayedLogCount = 0;
    updateLogDisplay();
    
    logUserOperation("Log viewer initialized");
}

// ===== 第四页：日志查看槽函数实现 =====

void MainWindow::onLogLevelChanged(int index)
{
    // 根据下拉框索引设置当前日志级别
    QVariant data = ui->comboBox_2->itemData(index);
    if (data.isValid()) {
        currentLogLevel = static_cast<LogLevel>(data.toInt());
        
        // 立即更新显示
        updateLogDisplay();
        
        QString levelName = ui->comboBox_2->currentText();
        logUserOperation(QString("Log level changed to: %1").arg(levelName));
        showStatusMessage(QString("日志级别已切换到: %1").arg(levelName));
    }
}

void MainWindow::onClearLogClicked()
{
    // 显示确认对话框
    bool confirmed = ConfirmDialog::confirm(
        this,
        "确定要清空所有日志吗？\n此操作无法撤销。",
        "确认清空日志",
        "清空",
        "取消"
    );
    
    if (confirmed) {
        // 清空Logger的内存日志
        Logger::getInstance().clearMemoryLogs();
        
        // 清空显示
        ui->textEdit_2->clear();
        lastDisplayedLogCount = 0;
        
        // 记录此操作
        logUserOperation("System logs cleared");
        showStatusMessage("系统日志已清空");
        
        // 立即更新显示（会显示刚才记录的清空操作）
        QTimer::singleShot(100, this, &MainWindow::updateLogDisplay);
    }
}

void MainWindow::onSaveLogClicked()
{
    // 获取当前筛选的日志
    std::vector<LogEntry> logsToSave;
    
    if (currentLogLevel == LogLevel::ALL) {
        logsToSave = Logger::getInstance().getRecentLogs(0); // 获取所有日志
    } else {
        logsToSave = Logger::getInstance().getLogsByLevel(currentLogLevel);
    }
    
    if (logsToSave.empty()) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, 
                              "当前没有日志可保存");
        return;
    }
    
    // 生成默认文件名
    QString defaultFilename = generateLogFilename();
    
    // 使用系统文件对话框选择保存位置
    QString filename = QFileDialog::getSaveFileName(
        this,
        "保存系统日志",
        defaultFilename,
        "文本文件 (*.txt);;日志文件 (*.log);;所有文件 (*.*)"
    );
    
    if (filename.isEmpty()) {
        return; // 用户取消
    }
    
    // 保存日志到文件
    if (saveLogsToFile(logsToSave, filename)) {
        QString levelName = ui->comboBox_2->currentText();
        QString message = QString("成功保存 %1 条%2级别的日志到文件")
                         .arg(logsToSave.size())
                         .arg(levelName);
        
        QMessageBox::information(this, "保存成功", message);
        
        logUserOperation(QString("System logs saved: %1 entries (%2 level) to %3")
                        .arg(logsToSave.size())
                        .arg(levelName)
                        .arg(QFileInfo(filename).fileName()));
        
        showStatusMessage(QString("日志已保存: %1 条记录").arg(logsToSave.size()));
    } else {
        ErrorDialog::showError(this, ErrorDialog::FileOperationError, 
                              QString("保存日志文件失败: %1").arg(filename));
    }
}

// ===== 第四页：辅助方法实现 =====

void MainWindow::updateLogDisplay()
{
    // 获取当前筛选级别的日志
    std::vector<LogEntry> logs;
    
    if (currentLogLevel == LogLevel::ALL) {
        logs = Logger::getInstance().getRecentLogs(0); // 获取所有日志
    } else {
        logs = Logger::getInstance().getLogsByLevel(currentLogLevel);
    }
    
    // 检查是否有新日志
    if (logs.size() <= lastDisplayedLogCount) {
        return; // 没有新日志
    }
    
    // 如果日志数量差异很大，可能是日志被清空后重新开始，需要重新显示所有日志
    if (logs.size() < lastDisplayedLogCount) {
        ui->textEdit_2->clear();
        lastDisplayedLogCount = 0;
    }
    
    // 添加新的日志条目
    for (size_t i = lastDisplayedLogCount; i < logs.size(); ++i) {
        QString logLine = formatLogEntry(logs[i]);
        appendLogToDisplay(logLine, logs[i].level);
    }
    
    // 更新已显示的日志数量
    lastDisplayedLogCount = logs.size();
    
    // 自动滚动到底部
    QTextCursor cursor = ui->textEdit_2->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEdit_2->setTextCursor(cursor);
}

QString MainWindow::formatLogEntry(const LogEntry& entry) const
{
    QString timestamp = formatLogTimestamp(entry.timestamp);
    QString level = QString::fromStdString(Logger::levelToString(entry.level));
    QString category = QString::fromStdString(entry.category);
    QString message = QString::fromStdString(entry.message);
    
    // 格式：[时间] [级别] [分类] 内容
    return QString("[%1] [%2] [%3] %4")
           .arg(timestamp)
           .arg(level)
           .arg(category)
           .arg(message);
}

QString MainWindow::formatLogTimestamp(const std::chrono::system_clock::time_point& time) const
{
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        time.time_since_epoch()).count() % 1000;
    
    QDateTime dateTime = QDateTime::fromSecsSinceEpoch(time_t);
    return QString("%1.%2")
           .arg(dateTime.toString("yyyy-MM-dd hh:mm:ss"))
           .arg(ms, 3, 10, QChar('0'));
}

void MainWindow::appendLogToDisplay(const QString& logLine, LogLevel level)
{
    // 根据日志级别设置不同颜色
    QColor textColor;
    switch (level) {
        case LogLevel::ERROR:
            textColor = QColor(220, 50, 50);   // 红色
            break;
        case LogLevel::WARNING:
            textColor = QColor(255, 140, 0);   // 橙色
            break;
        case LogLevel::INFO:
            textColor = QColor(70, 130, 180);  // 钢蓝色
            break;
        default:
            textColor = QColor(0, 0, 0);       // 黑色
            break;
    }
    
    // 设置文本格式
    QTextCharFormat format;
    format.setForeground(textColor);
    
    // 添加到显示区域
    QTextCursor cursor = ui->textEdit_2->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(logLine + "\n", format);
}

QString MainWindow::generateLogFilename() const
{
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyyMMdd_hhmmss");
    QString levelName = ui->comboBox_2->currentText().toLower();
    
    return QString("system_log_%1_%2.txt").arg(levelName).arg(timestamp);
}

bool MainWindow::saveLogsToFile(const std::vector<LogEntry>& logs, const QString& filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8); // 确保中文正确保存
    
    // 写入文件头
    out << "CDC控制系统 - 系统日志\n";
    out << "生成时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
    out << "日志级别: " << ui->comboBox_2->currentText() << "\n";
    out << "记录数量: " << logs.size() << "\n";
    out << QString("=").repeated(80) << "\n\n";
    
    // 写入日志条目
    for (const auto& entry : logs) {
        QString logLine = formatLogEntry(entry);
        out << logLine << "\n";
    }
    
    // 写入文件尾
    out << "\n" << QString("=").repeated(80) << "\n";
    out << "日志文件结束\n";
    
    file.close();
    return true;
}

void MainWindow::forceLogUpdate()
{
    // 强制立即更新日志显示（在重要操作后调用）
    updateLogDisplay();
}

// ===== 修改现有方法，添加强制日志更新 =====

// 修改logUserOperation方法，在记录后强制更新显示
void MainWindow::logUserOperation(const QString& operation)
{
    Logger::getInstance().logOperation("MainWindow", operation.toStdString());
    
    // 强制更新日志显示
    if (isInitialized) {
        QTimer::singleShot(50, this, &MainWindow::forceLogUpdate);
    }
}

// 在设备连接/断开时也强制更新
void MainWindow::onSerialConnectionChanged(bool connected)
{
    DeviceInfo* device = getCurrentSelectedDevice();
    if (device) {
        if (connected) {
            device->setConnectionStatus(ConnectionStatus::CONNECTED);
            showStatusMessage(QString("设备 '%1' 连接成功")
                             .arg(QString::fromStdString(device->getName())));
            
            // 启动传感器监控
            startSensorMonitoring();
        } else {
            device->setConnectionStatus(ConnectionStatus::DISCONNECTED);
            showStatusMessage(QString("设备 '%1' 连接断开")
                             .arg(QString::fromStdString(device->getName())));
            
            // 停止传感器监控
            stopSensorMonitoring();
        }
        
        updateDeviceListWidget();
        updateSelectedDeviceDisplay();
        updateDeviceButtons();
        updateMotorControlButtons();
        
        // 强制更新日志显示
        forceLogUpdate();
    }
}

// 增强的错误记录方法
void MainWindow::logError(const QString& error, const QString& context)
{
    QString fullMessage = context.isEmpty() ? error : QString("[%1] %2").arg(context, error);
    Logger::getInstance().error(fullMessage.toStdString());
    
    // 强制更新日志显示
    if (isInitialized) {
        QTimer::singleShot(50, this, &MainWindow::forceLogUpdate);
    }
}

void MainWindow::logWarning(const QString& warning, const QString& context)
{
    QString fullMessage = context.isEmpty() ? warning : QString("[%1] %2").arg(context, warning);
    Logger::getInstance().warning(fullMessage.toStdString());
    
    // 强制更新日志显示
    if (isInitialized) {
        QTimer::singleShot(50, this, &MainWindow::forceLogUpdate);
    }
}

void MainWindow::logInfo(const QString& info, const QString& context)
{
    QString fullMessage = context.isEmpty() ? info : QString("[%1] %2").arg(context, info);
    Logger::getInstance().info(fullMessage.toStdString());
    
    // 强制更新日志显示
    if (isInitialized) {
        QTimer::singleShot(50, this, &MainWindow::forceLogUpdate);
    }
}

// // ===== 第五页：数据可视化初始化 =====

// void MainWindow::initializeDataVisualization()
// {
//     // 创建CSV分析器
//     csvAnalyzer = std::make_unique<CsvAnalyzer>();
    
//     // 创建图表显示控件并添加到frame中
//     chartDisplayWidget = new ChartDisplayWidget(ui->frame);
//     QVBoxLayout* frameLayout = new QVBoxLayout(ui->frame);
//     frameLayout->setContentsMargins(0, 0, 0, 0);
//     frameLayout->addWidget(chartDisplayWidget);
//     ui->frame->setLayout(frameLayout);
    
//     // 设置UI
//     setupDataVisualizationUI();
    
//     // 连接信号槽
//     connectDataVisualizationSignals();
    
//     // 初始化状态
//     currentChartType = ChartType::DISTANCE_CAPACITANCE;
//     currentDataSource = DataSource::CSV_FILE;
//     isLoadingData = false;

//     // 创建3D图表控件（仅在3D模式时显示）
//     chart3DWidget = new Chart3DWidget(this);
//     chart3DWidget->setVisible(false);  // 初始隐藏
    
//     // 添加到布局
//     frameLayout->addWidget(chartDisplayWidget);  // 2D图表
//     frameLayout->addWidget(chart3DWidget);       // 3D图表
    
//     // 更新初始显示
//     updateChartStatistics();
    
//     logUserOperation("Data visualization initialized");
// }

// void MainWindow::setupDataVisualizationUI()
// {
//     // 设置图表类型下拉框
//     ui->comboBox_3->clear();
//     ui->comboBox_3->addItem("距离-电容分析", static_cast<int>(ChartType::DISTANCE_CAPACITANCE));
//     ui->comboBox_3->addItem("角度-电容分析", static_cast<int>(ChartType::ANGLE_CAPACITANCE));
//     ui->comboBox_3->addItem("3D 距离-角度-电容", static_cast<int>(ChartType::DISTANCE_ANGLE_CAPACITANCE_3D));
//     ui->comboBox_3->addItem("温度-电容分析", static_cast<int>(ChartType::TEMPERATURE_CAPACITANCE));
//     ui->comboBox_3->addItem("误差分析", static_cast<int>(ChartType::ERROR_ANALYSIS));
    
//     // 设置数据源下拉框
//     ui->comboBox_4->clear();
//     ui->comboBox_4->addItem("CSV文件", static_cast<int>(DataSource::CSV_FILE));
//     ui->comboBox_4->addItem("当前记录数据", static_cast<int>(DataSource::REAL_TIME));
    
//     // 设置范围控件
//     ui->doubleSpinBox_4->setRange(-1000.0, 1000.0);
//     ui->doubleSpinBox_4->setValue(0.0);
//     ui->doubleSpinBox_4->setSuffix(" mm");
    
//     ui->doubleSpinBox_5->setRange(-1000.0, 1000.0);
//     ui->doubleSpinBox_5->setValue(100.0);
//     ui->doubleSpinBox_5->setSuffix(" mm");
    
//     // 设置默认选项
//     ui->checkBox->setChecked(true);   // Show Grid
//     ui->checkBox_2->setChecked(true); // Auto Scale
    
//     // 初始化统计显示
//     ui->label_85->setText("0");      // Data Points
//     ui->label_87->setText("N/A");    // R² Value
//     ui->label_89->setText("N/A");    // RMSE
//     ui->label_91->setText("N/A");    // Max Error
//     ui->label_93->setText("N/A");    // Mean Error
// }

// void MainWindow::connectDataVisualizationSignals()
// {
//     // 图表类型选择
//     connect(ui->comboBox_3, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &MainWindow::onChartTypeChanged);
    
//     // 数据源选择
//     connect(ui->comboBox_4, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &MainWindow::onDataSourceChanged);
    
//     // 加载CSV按钮
//     connect(ui->pushButton_18, &QPushButton::clicked,
//             this, &MainWindow::onLoadCsvClicked);
    
//     // 图表设置
//     connect(ui->checkBox, &QCheckBox::toggled,
//             this, &MainWindow::onShowGridChanged);
    
//     connect(ui->checkBox_2, &QCheckBox::toggled,
//             this, &MainWindow::onAutoScaleChanged);
    
//     connect(ui->doubleSpinBox_4, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
//             this, &MainWindow::onChartRangeChanged);
    
//     connect(ui->doubleSpinBox_5, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
//             this, &MainWindow::onChartRangeChanged);
    
//     // 连接图表控件信号
//     if (chartDisplayWidget) {
//         connect(chartDisplayWidget, &ChartDisplayWidget::dataUpdated,
//                 this, &MainWindow::onChartDataUpdated);
        
//         connect(chartDisplayWidget, &ChartDisplayWidget::dataPointClicked,
//                 [this](double x, double y) {
//                     showStatusMessage(QString("数据点: X=%.2f, Y=%.2f").arg(x).arg(y));
//                 });
//     }
// }

// // ===== 第五页：槽函数实现 =====

// void MainWindow::onChartTypeChanged(int index)
// {
//     if (index < 0) return;
    
//     ChartType newType = static_cast<ChartType>(ui->comboBox_3->itemData(index).toInt());
//     currentChartType = newType;

//     if (type == ChartType::DISTANCE_ANGLE_CAPACITANCE_3D) {
//         // 显示3D图表，隐藏2D图表
//         chartDisplayWidget->setVisible(false);
//         chart3DWidget->setVisible(true);
        
//         // 设置3D数据
//         if (csvAnalyzer && csvAnalyzer->hasValidData()) {
//             auto data3D = csvAnalyzer->prepare3DData();
//             chart3DWidget->setData(data3D);
//         }
//     } else {
//         // 显示2D图表，隐藏3D图表
//         chartDisplayWidget->setVisible(true);
//         chart3DWidget->setVisible(false);
        
//         // 根据图表类型更新轴标签
//         ChartConfig config = chartDisplayWidget->getChartConfig();
        
//         switch (newType) {
//             case ChartType::DISTANCE_CAPACITANCE:
//                 config.xLabel = "距离 (mm)";
//                 config.yLabel = "电容 (pF)";
//                 config.title = "距离-电容关系图";
                
//                 // 提示用户选择固定条件
//                 if (csvAnalyzer && csvAnalyzer->hasValidData()) {
//                     showInfo(QString("当前固定条件：角度=%.1f°, 温度=%.1f°C")
//                             .arg(chartFixedConditions.fixedAngle)
//                             .arg(chartFixedConditions.fixedTemperature));
//                 }
//                 break;
                
//             case ChartType::ANGLE_CAPACITANCE:
//                 config.xLabel = "角度 (°)";
//                 config.yLabel = "电容 (pF)";
//                 config.title = "角度-电容关系图";
                
//                 if (csvAnalyzer && csvAnalyzer->hasValidData()) {
//                     showInfo(QString("当前固定条件：高度=%.1fmm, 温度=%.1f°C")
//                             .arg(chartFixedConditions.fixedHeight)
//                             .arg(chartFixedConditions.fixedTemperature));
//                 }
//                 break;
                
//             case ChartType::TEMPERATURE_CAPACITANCE:
//                 config.xLabel = "温度 (°C)";
//                 config.yLabel = "电容 (pF)";
//                 config.title = "温度-电容关系图";
                
//                 if (csvAnalyzer && csvAnalyzer->hasValidData()) {
//                     showInfo(QString("当前固定条件：高度=%.1fmm, 角度=%.1f°")
//                             .arg(chartFixedConditions.fixedHeight)
//                             .arg(chartFixedConditions.fixedAngle));
//                 }
//                 break;
                
//             case ChartType::ERROR_ANALYSIS:
//                 config.xLabel = "理论电容 (pF)";
//                 config.yLabel = "测量电容 (pF)";
//                 config.title = "理论值 vs 测量值";
//                 break;
                
//             case ChartType::DISTANCE_ANGLE_CAPACITANCE_3D:
//                 config.title = "3D 距离-角度-电容图";
//                 showInfo("3D图表：X轴=距离, Y轴=角度, Z轴=电容, 颜色=温度");
//                 break;
//         }
        
//         chartDisplayWidget->setChartConfig(config);
        
//         // 重新加载数据
//         if (csvAnalyzer && csvAnalyzer->hasValidData()) {
//             updateChartData();
//         }
//     }
    
//     logUserOperation(QString("Chart type changed to: %1")
//                     .arg(ui->comboBox_3->currentText()));
// }

// void MainWindow::onDataSourceChanged(int index)
// {
//     if (index < 0) return;
    
//     DataSource newSource = static_cast<DataSource>(ui->comboBox_4->itemData(index).toInt());
//     currentDataSource = newSource;
    
//     if (newSource == DataSource::REAL_TIME) {
//         // 使用当前记录的数据
//         if (dataRecorder && dataRecorder->hasData()) {
//             auto measurements = dataRecorder->getAllMeasurements();
//             if (csvAnalyzer) {
//                 csvAnalyzer->setData(measurements);
//                 updateChartData();
//                 showStatusMessage(QString("已加载 %1 条记录数据").arg(measurements.size()));
//             }
//         } else {
//             showWarning("当前没有记录的数据，请先在传感器监控页面记录数据");
//         }
//     } else {
//         // CSV文件模式
//         showInfo("请点击'Load CSV'按钮选择要分析的CSV文件");
//     }
    
//     logUserOperation(QString("Data source changed to: %1")
//                     .arg(ui->comboBox_4->currentText()));
// }

// void MainWindow::onLoadCsvClicked()
// {
//     if (isLoadingData) {
//         showWarning("正在加载数据，请稍候...");
//         return;
//     }
    
//     // 打开文件选择对话框
//     QString filename = QFileDialog::getOpenFileName(
//         this,
//         "选择CSV文件",
//         QDir::homePath(),
//         "CSV文件 (*.csv);;所有文件 (*.*)"
//     );
    
//     if (filename.isEmpty()) {
//         return;
//     }
    
//     isLoadingData = true;
//     showStatusMessage("正在加载CSV文件...");
    
//     try {
//         if (csvAnalyzer && csvAnalyzer->loadCsvFile(filename.toStdString())) {
//             size_t dataCount = csvAnalyzer->getDataCount();
            
//             // 更新图表
//             updateChartData();
            
//             // 更新统计信息
//             updateChartStatistics();
            
//             showStatusMessage(QString("成功加载 %1 条数据").arg(dataCount));
//             logUserOperation(QString("CSV file loaded: %1 (%2 records)")
//                            .arg(QFileInfo(filename).fileName())
//                            .arg(dataCount));
//         } else {
//             showError("无法加载CSV文件，请检查文件格式");
//         }
//     } catch (const std::exception& e) {
//         showError(QString("加载CSV文件失败: %1").arg(e.what()));
//     }
    
//     isLoadingData = false;
// }

// void MainWindow::onChartSettingsChanged()
// {
//     if (!chartDisplayWidget) return;
    
//     ChartConfig config = chartDisplayWidget->getChartConfig();
    
//     // 更新配置
//     config.showGrid = ui->checkBox->isChecked();
//     config.autoScale = ui->checkBox_2->isChecked();
    
//     chartDisplayWidget->setChartConfig(config);
    
//     // 如果不是自动缩放，应用手动范围
//     if (!config.autoScale) {
//         double min = ui->doubleSpinBox_4->value();
//         double max = ui->doubleSpinBox_5->value();
        
//         if (min < max) {
//             // 根据当前图表类型决定是设置X还是Y范围
//             chartDisplayWidget->setXRange(min, max);
//         }
//     }
// }

// void MainWindow::onShowGridChanged(bool checked)
// {
//     if (chartDisplayWidget) {
//         chartDisplayWidget->setShowGrid(checked);
//         logUserOperation(QString("Chart grid %1").arg(checked ? "enabled" : "disabled"));
//     }
// }

// void MainWindow::onAutoScaleChanged(bool checked)
// {
//     if (chartDisplayWidget) {
//         chartDisplayWidget->setAutoScale(checked);
        
//         // 启用/禁用范围控件
//         ui->doubleSpinBox_4->setEnabled(!checked);
//         ui->doubleSpinBox_5->setEnabled(!checked);
        
//         if (checked) {
//             chartDisplayWidget->autoFitRange();
//         }
        
//         logUserOperation(QString("Chart auto-scale %1").arg(checked ? "enabled" : "disabled"));
//     }
// }

// void MainWindow::onChartRangeChanged()
// {
//     if (!chartDisplayWidget || ui->checkBox_2->isChecked()) {
//         return; // 自动缩放模式下忽略
//     }
    
//     double min = ui->doubleSpinBox_4->value();
//     double max = ui->doubleSpinBox_5->value();
    
//     if (min >= max) {
//         showWarning("最小值必须小于最大值");
//         return;
//     }
    
//     // 根据当前图表类型设置范围
//     switch (currentChartType) {
//         case ChartType::DISTANCE_CAPACITANCE:
//         case ChartType::ANGLE_CAPACITANCE:
//         case ChartType::TEMPERATURE_CAPACITANCE:
//             chartDisplayWidget->setXRange(min, max);
//             break;
            
//         case ChartType::ERROR_ANALYSIS:
//             // 误差分析可能需要同时设置X和Y范围
//             chartDisplayWidget->setXRange(min, max);
//             chartDisplayWidget->setYRange(min, max);
//             break;
            
//         default:
//             break;
//     }
// }

// void MainWindow::updateChartStatistics()
// {
//     if (!csvAnalyzer || !csvAnalyzer->hasValidData()) {
//         // 清空统计显示
//         ui->label_85->setText("0");
//         ui->label_87->setText("N/A");
//         ui->label_89->setText("N/A");
//         ui->label_91->setText("N/A");
//         ui->label_93->setText("N/A");
//         return;
//     }
    
//     // 获取统计信息
//     auto stats = csvAnalyzer->calculateStatistics();
//     auto errorStats = csvAnalyzer->calculateErrorStatistics();
    
//     // 更新显示
//     ui->label_85->setText(QString::number(stats.dataCount));
//     ui->label_87->setText(QString::number(errorStats.r2Value, 'f', 4));
//     ui->label_89->setText(QString("%1 pF").arg(errorStats.rmse, 0, 'f', 2));
//     ui->label_91->setText(QString("%1 pF").arg(errorStats.maxError, 0, 'f', 2));
//     ui->label_93->setText(QString("%1 pF").arg(errorStats.meanError, 0, 'f', 2));
// }

// void MainWindow::onChartDataUpdated(int pointCount)
// {
//     // 更新数据点计数
//     ui->label_85->setText(QString::number(pointCount));
    
//     // 如果有数据，更新其他统计信息
//     if (pointCount > 0 && chartDisplayWidget) {
//         auto stats = chartDisplayWidget->getCurrentStatistics();
//         auto errorStats = chartDisplayWidget->getCurrentErrorStatistics();
        
//         // 更新R²值（如果适用）
//         if (currentChartType != ChartType::DISTANCE_ANGLE_CAPACITANCE_3D) {
//             ui->label_87->setText(QString::number(errorStats.r2Value, 'f', 4));
//         }
        
//         // 更新误差统计（仅在误差分析模式）
//         if (currentChartType == ChartType::ERROR_ANALYSIS) {
//             ui->label_89->setText(QString("%1 pF").arg(errorStats.rmse, 0, 'f', 2));
//             ui->label_91->setText(QString("%1 pF").arg(errorStats.maxError, 0, 'f', 2));
//             ui->label_93->setText(QString("%1 pF").arg(errorStats.meanError, 0, 'f', 2));
//         }
//     }
// }

// // ===== 私有辅助方法 =====

// void MainWindow::updateChartData()
// {
//     if (!csvAnalyzer || !csvAnalyzer->hasValidData() || !chartDisplayWidget) {
//         return;
//     }
    
//     std::vector<DataPoint> dataPoints;
    
//     // 根据图表类型准备数据
//     switch (currentChartType) {
//         case ChartType::DISTANCE_CAPACITANCE:
//             dataPoints = csvAnalyzer->prepareDistanceCapacitanceData(
//                 chartFixedConditions.fixedAngle,
//                 chartFixedConditions.fixedTemperature,
//                 2.0, 1.0  // 容差
//             );
//             break;
            
//         case ChartType::ANGLE_CAPACITANCE:
//             dataPoints = csvAnalyzer->prepareAngleCapacitanceData(
//                 chartFixedConditions.fixedHeight,
//                 chartFixedConditions.fixedTemperature,
//                 2.0, 1.0
//             );
//             break;
            
//         case ChartType::TEMPERATURE_CAPACITANCE:
//             dataPoints = csvAnalyzer->prepareTemperatureCapacitanceData(
//                 chartFixedConditions.fixedHeight,
//                 chartFixedConditions.fixedAngle,
//                 2.0, 2.0
//             );
//             break;
            
//         case ChartType::ERROR_ANALYSIS:
//             dataPoints = csvAnalyzer->prepareErrorAnalysisData();
//             break;
            
//         case ChartType::DISTANCE_ANGLE_CAPACITANCE_3D:
//             // 3D数据需要特殊处理
//             handle3DChartData();
//             return;
//     }
    
//     // 设置数据到图表
//     chartDisplayWidget->setDataPoints(dataPoints);
    
//     // 如果启用了趋势线，执行回归分析
//     if (chartDisplayWidget->getChartConfig().showTrendLine && 
//         currentChartType != ChartType::DISTANCE_ANGLE_CAPACITANCE_3D) {
//         auto regression = chartDisplayWidget->performLinearRegression();
        
//         // 可以在状态栏显示回归方程
//         if (regression.r2 > 0) {
//             QString equation = QString("y = %.3fx + %.3f (R² = %.4f)")
//                               .arg(regression.slope)
//                               .arg(regression.intercept)
//                               .arg(regression.r2);
//             showStatusMessage(equation, 5000);
//         }
//     }
    
//     // 更新统计信息
//     updateChartStatistics();
// }

// void MainWindow::handle3DChartData()
// {
//     if (!csvAnalyzer || !chartDisplayWidget) {
//         return;
//     }
    
//     // 准备3D数据
//     auto data3D = chartDisplayWidget->prepare3DData();
    
//     // 这里可以集成3D渲染库（如Qt3D或OpenGL）
//     // 暂时显示提示信息
//     QString info = QString("3D数据准备完成：\n")
//                   + QString("X轴(距离): %1 个点\n").arg(data3D.xPoints.size())
//                   + QString("Y轴(角度): %1 个点\n").arg(data3D.yPoints.size())
//                   + QString("Z轴(电容): %1 个点\n").arg(data3D.zPoints.size())
//                   + QString("颜色(温度): %1 个值").arg(data3D.colorValues.size());
    
//     showInfo(info);
    
//     // TODO: 实现3D渲染
//     // 可以使用Qt3D或集成VTK等3D库
// }

// void MainWindow::showFixedConditionsDialog()
// {
//     if (!csvAnalyzer || !csvAnalyzer->hasValidData()) {
//         showWarning("请先加载数据");
//         return;
//     }
    
//     // 创建固定条件对话框
//     FixedConditionsDialog dialog(this);
    
//     // 设置可用值
//     dialog.setAvailableHeights(csvAnalyzer->getUniqueHeights());
//     dialog.setAvailableAngles(csvAnalyzer->getUniqueAngles());
//     dialog.setAvailableTemperatures(csvAnalyzer->getUniqueTemperatures());
    
//     // 设置当前条件
//     FixedConditionsDialog::Conditions conditions;
//     conditions.height = chartFixedConditions.fixedHeight;
//     conditions.angle = chartFixedConditions.fixedAngle;
//     conditions.temperature = chartFixedConditions.fixedTemperature;
//     dialog.setConditions(conditions);
    
//     // 显示对话框
//     if (dialog.exec() == QDialog::Accepted) {
//         conditions = dialog.getConditions();
        
//         // 更新固定条件
//         chartFixedConditions.fixedHeight = conditions.height;
//         chartFixedConditions.fixedAngle = conditions.angle;
//         chartFixedConditions.fixedTemperature = conditions.temperature;
        
//         // 重新加载数据
//         updateChartData();
        
//         logUserOperation(QString("Fixed conditions updated: H=%.1fmm, A=%.1f°, T=%.1f°C")
//                         .arg(conditions.height)
//                         .arg(conditions.angle)
//                         .arg(conditions.temperature));
//     }
// }

// // ===== 错误处理辅助方法 =====

// void MainWindow::showError(const QString& message)
// {
//     ErrorDialog::showError(this, ErrorDialog::DataValidationError, message);
//     logError(message, "DataVisualization");
// }

// void MainWindow::showWarning(const QString& message)
// {
//     QMessageBox::warning(this, "警告", message);
//     logWarning(message, "DataVisualization");
// }

// void MainWindow::showInfo(const QString& message)
// {
//     // 在状态栏显示信息，而不是弹出对话框
//     showStatusMessage(message, 5000);
//     logInfo(message, "DataVisualization");
// }

void MainWindow::setupResponsiveLayout() {
    // ========== 1. 设置主TabWidget自适应 ==========
    if (ui->tabWidget) {
        ui->tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    
    // ========== 2. 第一页：设备管理页面布局改造 ==========
    // 获取第一个标签页
    QWidget *deviceTab = ui->tabWidget->widget(0);
    if (deviceTab) {
        // 清除原有的固定布局（如果是在Designer中手动放置的）
        QLayout *oldLayout = deviceTab->layout();
        if (!oldLayout) {
            // 创建新的响应式布局
            QHBoxLayout *mainLayout = new QHBoxLayout(deviceTab);
            
            // 左侧面板：设备列表
            QWidget *leftPanel = new QWidget();
            QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
            
            // 添加设备列表
            leftLayout->addWidget(ui->listWidget);
            
            // 按钮组
            QHBoxLayout *buttonLayout = new QHBoxLayout();
            buttonLayout->addWidget(ui->pushButton_3);  // 添加设备
            buttonLayout->addWidget(ui->pushButton_4);  // 删除设备
            leftLayout->addLayout(buttonLayout);
            
            // 设置左侧面板约束
            leftPanel->setMinimumWidth(200);
            leftPanel->setMaximumWidth(350);
            
            // 右侧面板：详情和日志
            QWidget *rightPanel = new QWidget();
            QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
            
            // 设备信息区域
            QGroupBox *infoGroup = new QGroupBox("设备信息");
            QGridLayout *infoLayout = new QGridLayout(infoGroup);
            infoLayout->addWidget(ui->label_3, 0, 0, 1, 2);  // 设备名称显示
            infoLayout->addWidget(ui->pushButton_20, 1, 0);  // 连接
            infoLayout->addWidget(ui->pushButton, 1, 1);     // 断开
            infoGroup->setMaximumHeight(120);
            
            // 命令发送区域
            QHBoxLayout *cmdLayout = new QHBoxLayout();
            cmdLayout->addWidget(ui->lineEdit);
            cmdLayout->addWidget(ui->pushButton_2);
            
            // 通信日志
            QLabel *logLabel = new QLabel("通信日志:");
            ui->textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            
            // 组装右侧面板
            rightLayout->addWidget(infoGroup);
            rightLayout->addLayout(cmdLayout);
            rightLayout->addWidget(logLabel);
            rightLayout->addWidget(ui->textEdit);
            
            // 使用分割器
            QSplitter *splitter = new QSplitter(Qt::Horizontal);
            splitter->addWidget(leftPanel);
            splitter->addWidget(rightPanel);
            splitter->setStretchFactor(0, 1);  // 左侧占1份
            splitter->setStretchFactor(1, 3);  // 右侧占3份
            
            mainLayout->addWidget(splitter);
        }
    }
    
    // ========== 3. 第二页：电机控制页面布局 ==========
    QWidget *motorTab = ui->tabWidget->widget(1);
    if (motorTab && !motorTab->layout()) {
        QVBoxLayout *motorLayout = new QVBoxLayout(motorTab);
        
        // 状态显示区（固定高度）
        QGroupBox *statusGroup = new QGroupBox("当前状态");
        QHBoxLayout *statusLayout = new QHBoxLayout(statusGroup);
        statusLayout->addWidget(new QLabel("高度:"));
        statusLayout->addWidget(ui->label_11);
        statusLayout->addStretch();
        statusLayout->addWidget(new QLabel("角度:"));
        statusLayout->addWidget(ui->label_8);
        statusLayout->addStretch();
        statusLayout->addWidget(new QLabel("理论电容:"));
        statusLayout->addWidget(ui->label_13);
        statusGroup->setMaximumHeight(80);
        
        // 控制区域（中等高度）
        QGroupBox *controlGroup = new QGroupBox("运动控制");
        QGridLayout *controlGrid = new QGridLayout(controlGroup);
        
        // 目标设置
        controlGrid->addWidget(new QLabel("目标高度:"), 0, 0);
        controlGrid->addWidget(ui->doubleSpinBox, 0, 1);
        controlGrid->addWidget(ui->pushButton_5, 0, 2);
        
        controlGrid->addWidget(new QLabel("目标角度:"), 1, 0);
        controlGrid->addWidget(ui->doubleSpinBox_2, 1, 1);
        controlGrid->addWidget(ui->pushButton_9, 1, 2);
        
        // 让输入框可以伸缩
        controlGrid->setColumnStretch(1, 1);
        
        // 按钮行
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addWidget(ui->pushButton_7);   // Move
        btnLayout->addWidget(ui->pushButton_8);   // Home
        btnLayout->addWidget(ui->pushButton_6);   // Stop
        btnLayout->addStretch();                  // 弹性空间
        ui->pushButton_12->setStyleSheet("QPushButton { background-color: red; }");
        btnLayout->addWidget(ui->pushButton_12);  // Emergency Stop
        
        controlGrid->addLayout(btnLayout, 2, 0, 1, 3);
        
        // 安全限位（可折叠）
        QGroupBox *safetyGroup = new QGroupBox("安全限位设置");
        QGridLayout *safetyGrid = new QGridLayout(safetyGroup);
        safetyGrid->addWidget(new QLabel("最大高度:"), 0, 0);
        safetyGrid->addWidget(ui->doubleSpinBox_3, 0, 1);
        safetyGrid->addWidget(new QLabel("最小高度:"), 0, 2);
        safetyGrid->addWidget(ui->doubleSpinBox_8, 0, 3);
        safetyGrid->addWidget(new QLabel("最小角度:"), 1, 0);
        safetyGrid->addWidget(ui->doubleSpinBox_6, 1, 1);
        safetyGrid->addWidget(new QLabel("最大角度:"), 1, 2);
        safetyGrid->addWidget(ui->doubleSpinBox_7, 1, 3);
        safetyGrid->setColumnStretch(1, 1);
        safetyGrid->setColumnStretch(3, 1);
        
        // 组装页面
        motorLayout->addWidget(statusGroup);
        motorLayout->addWidget(controlGroup);
        motorLayout->addWidget(safetyGroup);
        motorLayout->addStretch();  // 底部弹性空间
    }
    
    // ========== 4. 第三页：传感器监控页面 ==========
    QWidget *sensorTab = ui->tabWidget->widget(2);
    if (sensorTab && !sensorTab->layout()) {
        QVBoxLayout *sensorLayout = new QVBoxLayout(sensorTab);
        
        // 使用分割器分割上下两部分
        QSplitter *vSplitter = new QSplitter(Qt::Vertical);
        
        // 上半部分：主要测量值
        QGroupBox *primaryGroup = new QGroupBox("主要测量值");
        QGridLayout *primaryGrid = new QGridLayout(primaryGroup);
        // ... 添加标签和显示 ...
        primaryGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        
        // 下半部分：详细数据
        QGroupBox *detailGroup = new QGroupBox("详细传感器数据");
        detailGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
        vSplitter->addWidget(primaryGroup);
        vSplitter->addWidget(detailGroup);
        vSplitter->setStretchFactor(0, 1);
        vSplitter->setStretchFactor(1, 2);
        
        // 按钮区域
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addWidget(ui->pushButton_13);  // Record
        btnLayout->addWidget(ui->pushButton_14);  // Export
        btnLayout->addStretch();
        btnLayout->addWidget(ui->label_38);       // Last record time
        
        sensorLayout->addWidget(vSplitter);
        sensorLayout->addLayout(btnLayout);
    }
    
    // ========== 5. 第四页：日志查看页面 ==========
    QWidget *logTab = ui->tabWidget->widget(3);
    if (logTab && !logTab->layout()) {
        QVBoxLayout *logLayout = new QVBoxLayout(logTab);
        
        // 工具栏
        QHBoxLayout *toolbarLayout = new QHBoxLayout();
        toolbarLayout->addWidget(new QLabel("日志级别:"));
        toolbarLayout->addWidget(ui->comboBox_2);
        toolbarLayout->addStretch();
        toolbarLayout->addWidget(ui->pushButton_15);  // Clear
        toolbarLayout->addWidget(ui->pushButton_10);  // Save
        
        // 日志显示区域（可扩展）
        ui->textEdit_2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
        logLayout->addLayout(toolbarLayout);
        logLayout->addWidget(ui->textEdit_2);
    }
}

// ============================================
// 添加响应窗口大小变化的函数
// ============================================

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);  // 调用基类实现
    
    // 获取新的窗口大小
    QSize newSize = event->size();
    
    // 根据窗口大小调整布局
    adjustLayoutForSize(newSize);
}

void MainWindow::adjustLayoutForSize(const QSize &size) {
    int width = size.width();
    int height = size.height();
    
    // ===== 小屏幕模式 (宽度 < 1024) =====
    if (width < 1024) {
        // 调整字体大小
        QFont font = qApp->font();
        font.setPointSize(8);
        qApp->setFont(font);
        
        // 隐藏一些次要信息
        if (ui->label_19) ui->label_19->setVisible(false);
        if (ui->label_20) ui->label_20->setVisible(false);
        
        // 缩小按钮
        const QList<QPushButton*> buttons = findChildren<QPushButton*>();
        for (QPushButton *btn : buttons) {
            btn->setMinimumHeight(24);
        }
        
        // 减小间距
        if (centralWidget() && centralWidget()->layout()) {
            centralWidget()->layout()->setSpacing(2);
            centralWidget()->layout()->setContentsMargins(5, 5, 5, 5);
        }
    }
    // ===== 标准模式 (1024 <= 宽度 < 1600) =====
    else if (width < 1600) {
        // 恢复标准字体
        QFont font = qApp->font();
        font.setPointSize(9);
        qApp->setFont(font);
        
        // 显示所有信息
        if (ui->label_19) ui->label_19->setVisible(true);
        if (ui->label_20) ui->label_20->setVisible(true);
        
        // 标准按钮大小
        const QList<QPushButton*> buttons = findChildren<QPushButton*>();
        for (QPushButton *btn : buttons) {
            btn->setMinimumHeight(28);
        }
        
        // 标准间距
        if (centralWidget() && centralWidget()->layout()) {
            centralWidget()->layout()->setSpacing(6);
            centralWidget()->layout()->setContentsMargins(9, 9, 9, 9);
        }
    }
    // ===== 大屏幕模式 (宽度 >= 1600) =====
    else {
        // 增大字体
        QFont font = qApp->font();
        font.setPointSize(10);
        qApp->setFont(font);
        
        // 增大按钮
        const QList<QPushButton*> buttons = findChildren<QPushButton*>();
        for (QPushButton *btn : buttons) {
            btn->setMinimumHeight(32);
        }
        
        // 增大间距
        if (centralWidget() && centralWidget()->layout()) {
            centralWidget()->layout()->setSpacing(8);
            centralWidget()->layout()->setContentsMargins(12, 12, 12, 12);
        }
    }
    
    // 更新状态栏显示当前窗口大小（调试用）
    statusBar()->showMessage(QString("窗口大小: %1x%2").arg(width).arg(height), 2000);
}