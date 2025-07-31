#include "hardware/include/serial_interface.h"
#include "utils/include/logger.h"
#include <QDebug>

SerialInterface::SerialInterface(QObject *parent)
    : QObject(parent)
    , m_serialPort(std::make_unique<QSerialPort>())
{
    // 连接信号
    QObject::connect(m_serialPort.get(), &QSerialPort::readyRead,
                    this, &SerialInterface::handleReadyRead);
    QObject::connect(m_serialPort.get(), &QSerialPort::errorOccurred,
                    this, &SerialInterface::handleError);
}

SerialInterface::~SerialInterface() {
    if (isConnected()) {
        disconnect();
    }
}

QList<PortInfo> SerialInterface::getAvailablePorts() const {
    QList<PortInfo> ports;
    
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        PortInfo port;
        port.portName = info.portName();
        port.description = info.description();
        port.manufacturer = info.manufacturer();
        port.isBusy = info.isBusy();
        
        ports.append(port);
        
        LOG_DEBUG(QString("Found port: %1 - %2")
                 .arg(port.portName)
                 .arg(port.description));
    }
    
    return ports;
}

bool SerialInterface::connect(const QString& portName, int baudRate) {
    if (isConnected()) {
        LOG_WARNING("Already connected to a port");
        return false;
    }
    
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        LOG_INFO(QString("Connected to %1 at %2 baud")
                .arg(portName).arg(baudRate));
        emit connected(portName);
        return true;
    } else {
        QString error = m_serialPort->errorString();
        LOG_ERROR(QString("Failed to connect to %1: %2")
                 .arg(portName).arg(error));
        emit errorOccurred(error);
        return false;
    }
}

void SerialInterface::disconnect() {
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        m_receiveBuffer.clear();
        LOG_INFO("Serial port disconnected");
        emit disconnected();
    }
}

bool SerialInterface::isConnected() const {
    return m_serialPort->isOpen();
}

bool SerialInterface::sendData(const QString& data) {
    if (!isConnected()) {
        LOG_WARNING("Cannot send data: not connected");
        return false;
    }
    
    QByteArray bytes = data.toUtf8();
    qint64 written = m_serialPort->write(bytes);
    
    if (written == bytes.size()) {
        LOG_DEBUG(QString("Sent: %1").arg(data.trimmed()));
        return true;
    } else {
        LOG_ERROR("Failed to send all data");
        return false;
    }
}

bool SerialInterface::sendCommand(const QString& command) {
    return sendData(command + "\n");
}

void SerialInterface::handleReadyRead() {
    QByteArray data = m_serialPort->readAll();
    processReceivedData(data);
}

void SerialInterface::processReceivedData(const QByteArray& data) {
    m_receiveBuffer.append(data);
    emit dataReceived(data);
    
    processBuffer();
}

void SerialInterface::processBuffer() {
    // 查找完整的行
    int index;
    while ((index = m_receiveBuffer.indexOf('\n')) != -1) {
        // 提取一行（不包括换行符）
        QByteArray lineData = m_receiveBuffer.left(index);
        m_receiveBuffer.remove(0, index + 1);
        
        // 去除可能的\r
        if (lineData.endsWith('\r')) {
            lineData.chop(1);
        }
        
        if (!lineData.isEmpty()) {
            QString line = QString::fromUtf8(lineData);
            LOG_DEBUG(QString("Received: %1").arg(line));
            emit lineReceived(line);
        }
    }
    
    // 防止缓冲区过大
    if (m_receiveBuffer.size() > 4096) {
        LOG_WARNING("Receive buffer overflow, clearing");
        m_receiveBuffer.clear();
    }
}

void SerialInterface::handleError(QSerialPort::SerialPortError error) {
    if (error != QSerialPort::NoError) {
        QString errorStr = m_serialPort->errorString();
        LOG_ERROR(QString("Serial port error: %1").arg(errorStr));
        emit errorOccurred(errorStr);
    }
}

QString SerialInterface::getCurrentPort() const {
    return m_serialPort->portName();
}

int SerialInterface::getCurrentBaudRate() const {
    return m_serialPort->baudRate();
}

bool SerialInterface::setBaudRate(int baudRate) {
    return m_serialPort->setBaudRate(baudRate);
}

// ... 其他配置方法的实现类似