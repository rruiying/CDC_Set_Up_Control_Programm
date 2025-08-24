#include "../include/adddevicedialog.h"
#include "../../src/models/include/device_info.h"
#include "../../src/hardware/include/serial_interface.h"
#include "../../src/utils/include/logger.h"
#include "../include/errordialog.h"
#include "ui_adddevicedialog.h"

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
#include <QDateTime> 

QStringList AddDeviceDialog::cachedPorts;
QDateTime AddDeviceDialog::lastScanTime;

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
    setWindowTitle("add device");
    setFixedSize(400, 300);
    
    // 初始化定时器
    refreshTimer = new QTimer(this);
    refreshTimer->setSingleShot(false);
    refreshTimer->setInterval(5000);
    connect(refreshTimer, &QTimer::timeout, this, &AddDeviceDialog::onRefreshTimer);
    
    // 设置连接
    setupConnections();
    
    // 初始化波特率列表（端口列表在showEvent中初始化）
    initializeBaudRateList();
    
    // 设置设备名称验证器
//    QRegularExpression nameRegex("^[\\w\\s\\-_\\u4e00-\\u9fff]{1,50}$"); // 支持字母、数字、空格、连字符、下划线、中文
//    ui->lineEdit->setValidator(new QRegularExpressionValidator(nameRegex, this));
    
    // 初始状态验证
    //validateInput();
    
    logUserOperation("Open add device dialog");
}

QStringList AddDeviceDialog::getAvailablePortNames()
{
    if (lastScanTime.isValid() && 
        lastScanTime.msecsTo(QDateTime::currentDateTime()) < CACHE_VALIDITY_MS) {
        return cachedPorts;
    }
    
    QStringList portNames;
    auto ports = SerialInterface::getAvailablePorts();
    
    for (const auto& port : ports) {
        if (port.isAvailable) {
            portNames << QString::fromStdString(port.portName);
        }
    }
    
    cachedPorts = portNames;
    lastScanTime = QDateTime::currentDateTime();
    
    return portNames;
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

void AddDeviceDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    
    if (!isInitialized) {
        ui->comboBox->clear();
        ui->comboBox->addItem("Loading ports...", "");
        ui->comboBox->setEnabled(false);

        QTimer::singleShot(100, this, [this]() {
            initializePortList();
            ui->comboBox->setEnabled(true);
            
            refreshTimer->start();
            
            isInitialized = true;
            validateInput();
        });
    }
}

void AddDeviceDialog::setupConnections()
{
    connect(ui->lineEdit, &QLineEdit::textChanged, 
            this, &AddDeviceDialog::onDeviceNameChanged);
    
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddDeviceDialog::onPortSelectionChanged);
    
    connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddDeviceDialog::onBaudRateChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddDeviceDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddDeviceDialog::reject);
}

void AddDeviceDialog::initializePortList()
{
    ui->comboBox->clear();
    availablePorts.clear();
    
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
        
        if (isPortOccupiedByApp(portName)) {
            QString deviceName = getDeviceNameByPort(portName);
            displayText += QString(" [is used by '%1' ]").arg(deviceName);
            ui->comboBox->addItem(displayText, ""); 
        } else {
            ui->comboBox->addItem(displayText, portName);
            availablePorts << portName;
        }
    }
    
    if (availablePorts.isEmpty()) {
        ui->comboBox->addItem("No available ports", "");
        logUserOperation("No available ports found");
    } else {
        logUserOperation(QString("Found %1 available ports").arg(availablePorts.size()));
    }
    
    validateInput();
}

void AddDeviceDialog::initializeBaudRateList()
{
    if (ui->comboBox_2->count() == 0) {
        ui->comboBox_2->addItems({"9600", "19200", "38400", "57600", "115200"});
    }
    
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
    QString currentPort = ui->comboBox->currentData().toString();
    
    QStringList newPorts = getAvailablePortNames();
    
    if (newPorts != availablePorts) {
        initializePortList();
        
        if (!currentPort.isEmpty()) {
            int index = ui->comboBox->findData(currentPort);
            if (index >= 0) {
                ui->comboBox->setCurrentIndex(index);
            }
        }
        
        logUserOperation("Port list refreshed");
    }
}

void AddDeviceDialog::onRefreshTimer() {
    refreshPortList();
}

void AddDeviceDialog::validateInput() {
    if (!isInitialized) {
        return;
    }
    clearValidationError();

    // Validate device name
    auto nameValidation = validateDeviceName(ui->lineEdit->text());
    deviceNameValid = nameValidation.first;
    
    // Validate port selection
    auto portValidation = validatePortSelection();
    portSelectionValid = portValidation.first;
    
    // Baud rate is always valid (selected from predefined list)
    baudRateValid = (ui->comboBox_2->currentIndex() >= 0);
    
    // Show first error
    if (!deviceNameValid) {
        showValidationError(nameValidation.second);
    } else if (!portSelectionValid) {
        showValidationError(portValidation.second);
    }
    
    // Update OK button state
    updateOkButtonState();
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
    if (availablePorts.isEmpty()) {
        return {false, "no available ports"};
    }
    
    if (ui->comboBox->currentIndex() < 0) {
        return {false, "please select a serial port"};
    }
    
    QString selectedPort = ui->comboBox->currentData().toString();
    if (selectedPort.isEmpty()) {
        return {false, "the selected port is invalid or occupied"};
    }
    
    if (isPortOccupiedByApp(selectedPort)) {
        QString deviceName = getDeviceNameByPort(selectedPort);
        return {false, QString("the port is occupied by device '%1'").arg(deviceName)};
    }
    return {true, ""};
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
        // Could show error in status bar or label
        // For now, log to logger
        if (!message.isEmpty()) {
            Logger::getInstance().warning("Validation error: " + message.toStdString());
        }
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
        QString errorMsg = "Please check the input information:\n";
        if (!deviceNameValid) errorMsg += "- Invalid device name\n";
        if (!portSelectionValid) errorMsg += "- Invalid serial port selection\n";
        if (!baudRateValid) errorMsg += "- Invalid baud rate selection\n";

        ErrorDialog::showError(this, ErrorDialog::DataValidationError, errorMsg);
        return;
    }
    
    // 最终验证 - 实际尝试打开端口
    QString selectedPort = ui->comboBox->currentData().toString();
    if (selectedPort.isEmpty()) {
        ErrorDialog::showError(this, ErrorDialog::DataValidationError, "Please select a valid serial port");
        return;
    }
    
    // 再次检查端口是否被占用（可能在对话框打开期间被占用）
    if (isPortOccupiedByApp(selectedPort)) {
        QString deviceName = getDeviceNameByPort(selectedPort);
        ErrorDialog::showError(this, ErrorDialog::HardwareError, 
                             QString("the port %1 is occupied by device '%2'").arg(selectedPort, deviceName));
        refreshPortList(); // 刷新端口列表
        return;
    }
    
    // 实际测试端口可用性
//    try {
//        SerialInterface testInterface;
//        if (!testInterface.open(selectedPort.toStdString(), ui->comboBox_2->currentText().toInt())) {
//            ErrorDialog::showError(this, ErrorDialog::HardwareError, 
//                                 QString("the port %1 is occupied by another program or inaccessible").arg(selectedPort));
//            refreshPortList(); 
//            return;
//        }
        // 立即关闭测试连接
//        testInterface.close();
//    } catch (const std::exception& e) {
//        ErrorDialog::showError(this, ErrorDialog::HardwareError, 
//                             QString("the port test failed: %1").arg(e.what()));
 //       return;
//    } catch (...) {
//        ErrorDialog::showError(this, ErrorDialog::HardwareError, "the port test failed: unknown error");
//       return;
//    }

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
    Logger::getInstance().info("AddDeviceDialog", operation.toStdString());
}