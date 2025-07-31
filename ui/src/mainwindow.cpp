#include "ui/include/mainwindow.h"
#include "ui_mainwindow.h"
#include "utils/include/logger.h"
#include "utils/include/config_manager.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serialInterface(new SerialInterface(this))
{
    ui->setupUi(this);
    
    // 设置窗口标题
    setWindowTitle("CDC Setup Control Program");
    
    // 初始化UI
    setupUI();
    
    // 连接信号槽
    setupConnections();
    
    // 加载配置
    loadConfiguration();
    
    // 更新端口列表
    updatePortList();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupUI() {
    // 初始化状态栏
    ui->statusbar->showMessage("Ready");
    
    // 设置初始页面
    ui->tabWidget->setCurrentIndex(0);
    
    // 禁用未连接时的功能
    updateUIState(false);
}

void MainWindow::setupConnections() {
    // 串口通信页面
    connect(ui->pushButton_3, &QPushButton::clicked,
            this, &MainWindow::onAddDeviceClicked);
    connect(ui->pushButton, &QPushButton::clicked,
            this, &MainWindow::onDisconnectClicked);
    connect(ui->pushButton_2, &QPushButton::clicked,
            this, &MainWindow::onSendCommandClicked);
    
    // 串口信号
    connect(m_serialInterface, &SerialInterface::connected,
            this, &MainWindow::onSerialConnected);
    connect(m_serialInterface, &SerialInterface::disconnected,
            this, &MainWindow::onSerialDisconnected);
    connect(m_serialInterface, &SerialInterface::lineReceived,
            this, &MainWindow::onSerialDataReceived);
    connect(m_serialInterface, &SerialInterface::errorOccurred,
            this, &MainWindow::onSerialError);
    
    // Logger信号
    connect(&Logger::instance(), &Logger::newLogEntry,
            this, &MainWindow::onNewLogEntry);
    
    // 列表选择
    connect(ui->listWidget, &QListWidget::itemClicked,
            this, &MainWindow::onDeviceSelected);
}

void MainWindow::loadConfiguration() {
    ConfigManager& config = ConfigManager::instance();
    config.loadFromFile("config.json");
    
    LOG_INFO("Configuration loaded");
}

void MainWindow::updatePortList() {
    ui->listWidget->clear();
    
    auto ports = m_serialInterface->getAvailablePorts();
    for (const auto& port : ports) {
        QString item = QString("%1 - %2")
                      .arg(port.portName)
                      .arg(port.description);
        ui->listWidget->addItem(item);
    }
    
    if (ports.isEmpty()) {
        ui->listWidget->addItem("No serial ports found");
    }
}

void MainWindow::updateUIState(bool connected) {
    // 连接/断开按钮
    ui->pushButton_3->setEnabled(!connected);  // Add Device
    ui->pushButton->setEnabled(connected);      // Disconnect
    
    // 发送命令
    ui->pushButton_2->setEnabled(connected);   // Send
    ui->lineEdit->setEnabled(connected);
    
    // 其他标签页
    ui->tabWidget->setTabEnabled(1, connected); // Motor Control
    ui->tabWidget->setTabEnabled(2, connected); // Sensor Monitor
}

void MainWindow::onAddDeviceClicked() {
    // 获取选中的设备
    auto currentItem = ui->listWidget->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "Warning", "Please select a device first");
        return;
    }
    
    QString text = currentItem->text();
    QString portName = text.split(" - ").first();
    
    // 连接设备
    if (m_serialInterface->connect(portName, 115200)) {
        ui->label_3->setText(portName);
        LOG_INFO(QString("Connected to %1").arg(portName));
    } else {
        QMessageBox::critical(this, "Error", "Failed to connect to device");
    }
}

void MainWindow::onDisconnectClicked() {
    m_serialInterface->disconnect();
}

void MainWindow::onSendCommandClicked() {
    QString command = ui->lineEdit->text();
    if (!command.isEmpty()) {
        if (m_serialInterface->sendCommand(command)) {
            ui->textEdit->append(QString("> %1").arg(command));
            ui->lineEdit->clear();
        }
    }
}

void MainWindow::onSerialConnected(const QString& portName) {
    ui->statusbar->showMessage(QString("Connected to %1").arg(portName));
    updateUIState(true);
}

void MainWindow::onSerialDisconnected() {
    ui->statusbar->showMessage("Disconnected");
    ui->label_3->setText("No device selected");
    updateUIState(false);
}

void MainWindow::onSerialDataReceived(const QString& line) {
    // 显示在通信日志
    ui->textEdit->append(QString("< %1").arg(line));
    
    // 解析传感器数据
    SensorData data = SensorData::fromSerialData(line, m_lastSensorData);
    m_lastSensorData = data;
    
    // 更新显示
    updateSensorDisplay(data);
}

void MainWindow::onSerialError(const QString& error) {
    ui->statusbar->showMessage(QString("Error: %1").arg(error), 5000);
}

void MainWindow::onNewLogEntry(const QString& entry) {
    // 更新日志查看器页面
    if (ui->textEdit_2) {
        ui->textEdit_2->append(entry);
    }
}

void MainWindow::onDeviceSelected(QListWidgetItem* item) {
    // 用户选择了一个设备
    Q_UNUSED(item);
}

void MainWindow::updateSensorDisplay(const SensorData& data) {
    // 更新传感器监控页面
    if (data.isValid.temperature) {
        ui->label_27->setText(QString("%1°C").arg(data.temperature, 0, 'f', 1));
    }
    
    if (data.isValid.angle) {
        ui->label_25->setText(QString("%1°").arg(data.angle, 0, 'f', 1));
    }
    
    // ... 更新其他传感器显示
}