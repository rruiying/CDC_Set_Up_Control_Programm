// src/hardware/src/qt_serial_interface.cpp
#include "../include/qt_serial_interface.h"
#include "../../utils/include/logger.h"

QtSerialInterface::QtSerialInterface(QObject* parent)
    : QObject(parent)
    , m_serial(std::make_unique<SerialInterface>())
    , m_pollTimer(new QTimer(this))
{
    // 设置回调函数
    m_serial->setConnectionCallback([this](bool connected) {
        if (connected) {
            m_pollTimer->start(10); // 10ms轮询间隔
            emit this->connected();
        } else {
            m_pollTimer->stop();
            emit this->disconnected();
        }
    });
    
    m_serial->setErrorCallback([this](const std::string& error) {
        emit errorOccurred(QString::fromStdString(error));
    });
    
    // 连接轮询定时器
    connect(m_pollTimer, &QTimer::timeout, this, &QtSerialInterface::pollForData);
}

QtSerialInterface::~QtSerialInterface() {
    close();
}

bool QtSerialInterface::open(const QString& portName, int baudRate) {
    return m_serial->open(portName.toStdString(), baudRate);
}

bool QtSerialInterface::open(const QString& portName, const SerialPortConfig& config) {
    return m_serial->open(portName.toStdString(), config);
}

void QtSerialInterface::close() {
    m_serial->close();
}

QString QtSerialInterface::getCurrentPort() const {
    return QString::fromStdString(m_serial->getCurrentPort());
}

int QtSerialInterface::getCurrentBaudRate() const {
    return m_serial->getCurrentBaudRate();
}

bool QtSerialInterface::sendCommand(const QString& command) {
    // 添加换行符（如果需要）
    QString cmdToSend = command;
    if (!cmdToSend.endsWith('\n')) {
        cmdToSend += '\n';
    }
    
    return m_serial->sendCommand(cmdToSend.toStdString());
}

QString QtSerialInterface::sendAndReceive(const QString& command, int timeoutMs) {
    QString cmdToSend = command;
    if (!cmdToSend.endsWith('\n')) {
        cmdToSend += '\n';
    }
    
    std::string response = m_serial->sendAndReceive(cmdToSend.toStdString(), timeoutMs);
    return QString::fromStdString(response);
}

QStringList QtSerialInterface::getAvailablePorts() {
    QStringList ports;
    auto portInfos = SerialInterface::getAvailablePorts();
    
    for (const auto& info : portInfos) {
        if (info.isAvailable) {
            ports.append(QString::fromStdString(info.portName));
        }
    }
    
    return ports;
}

void QtSerialInterface::setAutoReconnect(bool enable) {
    m_serial->setAutoReconnect(enable);
}

void QtSerialInterface::pollForData() {
    // 检查是否有可用数据
    int bytesAvailable = m_serial->bytesAvailable();
    if (bytesAvailable > 0) {
        // 读取所有可用数据
        auto bytes = m_serial->readBytes(bytesAvailable, 0);
        if (!bytes.empty()) {
            QByteArray data(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            emit dataReceived(data);
            
            // 添加到缓冲区
            m_readBuffer.append(data);
            processReceivedData();
        }
    }
}

void QtSerialInterface::processReceivedData() {
    // 查找完整的行
    int index;
    while ((index = m_readBuffer.indexOf('\n')) != -1) {
        // 提取一行（包括换行符）
        QString line = m_readBuffer.left(index + 1);
        m_readBuffer.remove(0, index + 1);
        
        // 移除换行符
        line = line.trimmed();
        
        if (!line.isEmpty()) {
            emit lineReceived(line);
        }
    }
}