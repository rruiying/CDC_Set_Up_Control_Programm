// ui/src/adddevicedialog.cpp
#include "../include/adddevicedialog.h"
#include "../ui_adddevicedialog.h"
#include "../../src/models/include/device_info.h"
#include "../../src/hardware/include/serial_interface.h"
#include "../../src/utils/include/logger.h"
#include "../include/errordialog.h"

#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSerialPortInfo>
#include <QShowEvent>
#include <QTimer>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

AddDeviceDialog::AddDeviceDialog(QWidget *parent, const QStringList& existingDevices, 
                                 const QList<DeviceInfo>& connectedDevices)
    : QDialog(parent)
    , ui(new Ui::AddDeviceDialog)
    , existingDeviceNames(existingDevices)
    , connectedDevices(connectedDevices)
    , refreshTimer(nullptr)
    , deviceNameValid(false)
    , portSelectionValid(false)
    , baudRateValid(true) // 波特率默认有效，因为有预设值
    , isInitialized(false)
{
    ui->setupUi(this);
    
    // 设置窗口属性
    setModal(true);
    setWindowTitle("添加设备");
    setFixedSize(400, 300);
    
    // 初始化定时器
    refreshTimer = new QTimer(this);
    refreshTimer->setSingleShot(false);
    refreshTimer->setInterval(PORT_REFRESH_INTERVAL);
    connect(refreshTimer, &QTimer::timeout, this, &AddDeviceDialog::onRefreshTimer);
    
    // 设置连接
    setupConnections();
    
    // 初始化波特率列表（端口列表在showEvent中初始化）
    initializeBaudRateList();
    
    // 设置设备名称验证器
//    QRegularExpression nameRegex("^[\\w\\s\\-_\\u4e00-\\u9fff]{1,50}$"); // 支持字母、数字、空格、连字符、下划线、中文
//    ui->lineEdit->setValidator(new QRegularExpressionValidator(nameRegex, this));
    
    // 初始状态验证
    validateInput();
    
    logUserOperation("Open add device dialog");
}

AddDeviceDialog::~AddDeviceDialog()
{
    if (refreshTimer && refreshTimer->isActive()) {
        refreshTimer->stop();
    }
    delete ui;
}

DeviceInfo AddDeviceDialog::getDeviceInfo() const
{
    QString deviceName = ui->lineEdit->text().trimmed();
    QString portName = ui->comboBox->currentData().toString();
    if (portName.isEmpty() && ui->comboBox->count() > 0) {
        portName = ui->comboBox->currentText();
    }
    
    QString baudRateText = ui->comboBox_2->currentText();
    int baudRate = baudRateText.toInt();
    
    DeviceInfo info;
    info.setName(deviceName.toStdString());
    info.setPortName(portName.toStdString());
    info.setBaudRate(baudRate);
    info.setConnectionStatus(ConnectionStatus::DISCONNECTED);
    
    return info;
}

bool AddDeviceDialog::hasAvailablePorts()
{
    auto ports = SerialInterface::getAvailablePorts();
    for (const auto& port : ports) {
        if (port.isAvailable) {
            return true;
        }
    }
    return false;
}

QStringList AddDeviceDialog::getAvailablePortNames()
{
    QStringList portNames;
    auto ports = SerialInterface::getAvailablePorts();
    
    for (const auto& port : ports) {
        if (port.isAvailable) {
            portNames << QString::fromStdString(port.portName);
        }
    }
    
    return portNames;
}

void AddDeviceDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    
    if (!isInitialized) {
        // 第一次显示时初始化端口列表
        initializePortList();
        
        // 启动定时器定期刷新端口
        refreshTimer->start();
        
        isInitialized = true;
    }
}

void AddDeviceDialog::setupConnections()
{
    // 设备名称变化
    connect(ui->lineEdit, &QLineEdit::textChanged, 
            this, &AddDeviceDialog::onDeviceNameChanged);
    
    // 端口选择变化
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddDeviceDialog::onPortSelectionChanged);
    
    // 波特率变化
    connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddDeviceDialog::onBaudRateChanged);
    
    // 按钮连接
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddDeviceDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddDeviceDialog::reject);
}

void AddDeviceDialog::initializePortList()
{
    ui->comboBox->clear();
    availablePorts.clear();
    
    // 获取可用端口
    auto ports = SerialInterface::getAvailablePorts();
    for (const auto& portInfo : ports) {
        if (!portInfo.isAvailable) {
            continue;
        }
        
        QString portName = QString::fromStdString(portInfo.portName);
        QString description = QString::fromStdString(portInfo.description);
        QString displayText = portName;
        
        if (!description.isEmpty() && description != portName) {
            displayText += QString(" (%1)").arg(description);
        }
        
        // 检查端口是否被当前应用占用
        if (isPortOccupiedByApp(portName)) {
            QString deviceName = getDeviceNameByPort(portName);
            displayText += QString(" [被 '%1' 占用]").arg(deviceName);
            ui->comboBox->addItem(displayText, ""); // 数据设为空表示不可用
        } else {
            ui->comboBox->addItem(displayText, portName);
            availablePorts << portName;
        }
    }
    
    // 如果没有可用端口，显示提示
    if (availablePorts.isEmpty()) {
        ui->comboBox->addItem("未找到可用端口", "");
        logUserOperation("No available ports found");
    } else {
        logUserOperation(QString("Found %1 available ports").arg(availablePorts.size()));
    }
    
    // 验证端口选择
    validateInput();
}

void AddDeviceDialog::initializeBaudRateList()
{
    // UI文件中已经预设了波特率选项，这里确保正确性
    if (ui->comboBox_2->count() == 0) {
        ui->comboBox_2->addItems({"9600", "19200", "38400", "57600", "115200"});
    }
    
    // 默认选择115200
    int index = ui->comboBox_2->findText("115200");
    if (index >= 0) {
        ui->comboBox_2->setCurrentIndex(index);
    }
}

void AddDeviceDialog::onDeviceNameChanged(const QString& text)
{
    Q_UNUSED(text)
    validateInput();
}

void AddDeviceDialog::onPortSelectionChanged(int index)
{
    Q_UNUSED(index)
    validateInput();
    
    // 记录端口选择
    if (ui->comboBox->currentData().isValid()) {
        QString selectedPort = ui->comboBox->currentData().toString();
        logUserOperation(QString("Selected port: %1").arg(selectedPort));
    }
}

void AddDeviceDialog::onBaudRateChanged(int index)
{
    Q_UNUSED(index)
    validateInput();
    
    // 记录波特率选择
    QString selectedBaud = ui->comboBox_2->currentText();
    logUserOperation(QString("Selected baud rate: %1").arg(selectedBaud));
}

void AddDeviceDialog::refreshPortList()
{
    // 保存当前选择
    QString currentPort = ui->comboBox->currentData().toString();
    
    // 重新扫描端口
    QStringList newPorts = getAvailablePortNames();
    
    // 检查是否有变化
    if (newPorts != availablePorts) {
        // 更新端口列表
        initializePortList();
        
        // 尝试恢复之前的选择
        if (!currentPort.isEmpty()) {
            int index = ui->comboBox->findData(currentPort);
            if (index >= 0) {
                ui->comboBox->setCurrentIndex(index);
            }
        }
        
        logUserOperation("Port list refreshed");
    }
}

void AddDeviceDialog::onRefreshTimer()
{
    refreshPortList();
}

//void AddDeviceDialog::validateInput()
//{
//    clearValidationError();
//
//    // 验证设备名称
//    auto nameValidation = validateDeviceName(ui->lineEdit->text());
//    deviceNameValid = nameValidation.first;
//
//```
    // 验证端口选择
//    auto portValidation = validatePortSelection();
//    portSelectionValid = portValidation.first;
    
    // 波特率始终有效（从预定义列表选择）
//    baudRateValid = (ui->comboBox_2->currentIndex() >= 0);
    
    // 显示第一个错误
//    if (!deviceNameValid) {
//        showValidationError(nameValidation.second);
//    } else if (!portSelectionValid) {
//        showValidationError(portValidation.second);
//    }
    
    // 更新OK按钮状态
//    updateOkButtonState();
//}

void AddDeviceDialog::validateInput()
{
    // 只保留最基本的逻辑
    deviceNameValid = !ui->lineEdit->text().trimmed().isEmpty();
    portSelectionValid = true;
    baudRateValid = true;
    
    // 暂时注释掉可能有问题的函数调用
    // clearValidationError();
    // showValidationError(...);
    // updateOkButtonState();
    
    // 手动设置OK按钮状态
    bool allValid = deviceNameValid && portSelectionValid && baudRateValid;
    if (ui->buttonBox) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allValid);
    }
}

QPair<bool, QString> AddDeviceDialog::validateDeviceName(const QString& name) const
{
    QString trimmedName = name.trimmed();
    
    // 检查是否为空
    if (trimmedName.isEmpty()) {
        return {false, "设备名称不能为空"};
    }
    
    // 检查长度
    if (trimmedName.length() > MAX_DEVICE_NAME_LENGTH) {
        return {false, QString("设备名称不能超过%1个字符").arg(MAX_DEVICE_NAME_LENGTH)};
    }
    
    // 检查是否与现有设备重复
    if (existingDeviceNames.contains(trimmedName, Qt::CaseInsensitive)) {
        return {false, "设备名称已存在"};
    }
    
    // 检查字符有效性（由validator处理，这里做额外检查）
//    QRegularExpression validChars("^[\\w\\s\\-_\\u4e00-\\u9fff]+$");
//    if (!validChars.match(trimmedName).hasMatch()) {
//        return {false, "设备名称包含无效字符"};
//    }
    
    return {true, ""};
}

QPair<bool, QString> AddDeviceDialog::validatePortSelection() const
{
    // 检查是否有可用端口
    if (availablePorts.isEmpty()) {
        return {false, "未找到可用的串口设备"};
    }
    
    // 检查是否选择了端口
    if (ui->comboBox->currentIndex() < 0) {
        return {false, "请选择一个串口"};
    }
    
    QString selectedPort = ui->comboBox->currentData().toString();
    if (selectedPort.isEmpty()) {
        return {false, "选择的端口无效或已被占用"};
    }
    
    // 检查端口是否被当前应用占用
    if (isPortOccupiedByApp(selectedPort)) {
        QString deviceName = getDeviceNameByPort(selectedPort);
        return {false, QString("端口被设备 '%1' 占用").arg(deviceName)};
    }
    
    // 实际尝试打开端口验证
    try {
        SerialInterface testInterface;
        if (!testInterface.open(selectedPort.toStdString(), 115200)) {
            return {false, "端口被其他程序占用或无法访问"};
        }
        // 立即关闭测试连接
        testInterface.close();
        
        return {true, ""};
    } catch (const std::exception& e) {
        return {false, QString("端口测试失败: %1").arg(e.what())};
    } catch (...) {
        return {false, "端口测试失败: 未知错误"};
    }
}

bool AddDeviceDialog::isPortOccupiedByApp(const QString& portName) const
{
    for (const auto& device : connectedDevices) {
        if (device.isConnected() && 
            QString::fromStdString(device.getPortName()) == portName) {
            return true;
        }
    }
    return false;
}

QString AddDeviceDialog::getDeviceNameByPort(const QString& portName) const
{
    for (const auto& device : connectedDevices) {
        if (device.isConnected() && 
            QString::fromStdString(device.getPortName()) == portName) {
            return QString::fromStdString(device.getName());
        }
    }
    return "";
}

void AddDeviceDialog::updateOkButtonState()
{
    QPushButton* okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        bool allValid = deviceNameValid && portSelectionValid && baudRateValid;
        okButton->setEnabled(allValid);
    }
}

void AddDeviceDialog::showValidationError(const QString& message)
{
    if (message != lastValidationError) {
        lastValidationError = message;
        // 可以在状态栏或标签中显示错误消息
        // 这里记录到日志
        Logger::getInstance().warning("Validation error: " + message.toStdString());
    }
}

void AddDeviceDialog::clearValidationError()
{
    lastValidationError.clear();
}

void AddDeviceDialog::accept()
{
    // 最终验证
    validateInput();
    
    if (!deviceNameValid || !portSelectionValid || !baudRateValid) {
        QString errorMsg = "请检查输入信息：\n";
        if (!deviceNameValid) errorMsg += "- 设备名称无效\n";
        if (!portSelectionValid) errorMsg += "- 串口选择无效\n";
        if (!baudRateValid) errorMsg += "- 波特率选择无效\n";
        
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, errorMsg);
        return;
    }
    
    // 最终验证 - 实际尝试打开端口
    QString selectedPort = ui->comboBox->currentData().toString();
    if (selectedPort.isEmpty()) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, "请选择有效的串口");
        return;
    }
    
    // 再次检查端口是否被占用（可能在对话框打开期间被占用）
    if (isPortOccupiedByApp(selectedPort)) {
        QString deviceName = getDeviceNameByPort(selectedPort);
        ErrorDialog::showError(this, ErrorDialog::HardwareError, 
                             QString("端口 %1 已被设备 '%2' 占用").arg(selectedPort, deviceName));
        refreshPortList(); // 刷新端口列表
        return;
    }
    
    // 实际测试端口可用性
    try {
        SerialInterface testInterface;
        if (!testInterface.open(selectedPort.toStdString(), ui->comboBox_2->currentText().toInt())) {
            ErrorDialog::showError(this, ErrorDialog::HardwareError, 
                                 QString("无法打开端口 %1，可能被其他程序占用").arg(selectedPort));
            refreshPortList(); // 刷新端口列表
            return;
        }
        // 立即关闭测试连接
        testInterface.close();
    } catch (const std::exception& e) {
        ErrorDialog::showError(this, ErrorDialog::HardwareError, 
                             QString("端口测试失败: %1").arg(e.what()));
        return;
    } catch (...) {
        ErrorDialog::showError(this, ErrorDialog::HardwareError, "端口测试失败: 未知错误");
        return;
    }
    
    // 停止定时器
    if (refreshTimer && refreshTimer->isActive()) {
        refreshTimer->stop();
    }
    
    // 记录成功添加
    DeviceInfo info = getDeviceInfo();
    logUserOperation(QString("Device added successfully: %1 on %2 at %3")
                    .arg(QString::fromStdString(info.getName()))
                    .arg(QString::fromStdString(info.getPortName()))
                    .arg(info.getBaudRate()));
    
    QDialog::accept();
}

void AddDeviceDialog::reject()
{
    // 停止定时器
    if (refreshTimer && refreshTimer->isActive()) {
        refreshTimer->stop();
    }
    
    logUserOperation("Add device dialog cancelled");
    QDialog::reject();
}

void AddDeviceDialog::logUserOperation(const QString& operation)
{
    Logger::getInstance().logOperation("AddDeviceDialog", operation.toStdString());
}