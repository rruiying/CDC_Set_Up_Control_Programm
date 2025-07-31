#pragma once
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QString>
#include <QByteArray>
#include <memory>

struct PortInfo {
    QString portName;
    QString description;
    QString manufacturer;
    bool isBusy;
};

class SerialInterface : public QObject {
    Q_OBJECT
    
public:
    explicit SerialInterface(QObject *parent = nullptr);
    ~SerialInterface();
    
    // 端口管理
    QList<PortInfo> getAvailablePorts() const;
    bool connect(const QString& portName, int baudRate = 115200);
    void disconnect();
    bool isConnected() const;
    
    // 数据传输
    bool sendData(const QString& data);
    bool sendCommand(const QString& command);  // 自动添加换行符
    
    // 配置
    bool setBaudRate(int baudRate);
    bool setDataBits(QSerialPort::DataBits dataBits);
    bool setParity(QSerialPort::Parity parity);
    bool setStopBits(QSerialPort::StopBits stopBits);
    bool setFlowControl(QSerialPort::FlowControl flowControl);
    
    // 状态
    QString getCurrentPort() const;
    int getCurrentBaudRate() const;
    
    // 测试用：处理接收的数据
    void processReceivedData(const QByteArray& data);
    
signals:
    // 数据信号
    void dataReceived(const QByteArray& data);  // 原始数据
    void lineReceived(const QString& line);     // 完整的一行
    
    // 状态信号
    void connected(const QString& portName);
    void disconnected();
    void errorOccurred(const QString& error);
    
private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    
private:
    std::unique_ptr<QSerialPort> m_serialPort;
    QByteArray m_receiveBuffer;  // 接收缓冲区
    
    void processBuffer();  // 处理缓冲区中的完整行
};