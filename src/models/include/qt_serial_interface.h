// src/hardware/include/qt_serial_interface.h
#ifndef QT_SERIAL_INTERFACE_H
#define QT_SERIAL_INTERFACE_H

#include <QObject>
#include <QTimer>
#include <memory>
#include "serial_interface.h"

/**
 * @brief Qt包装的串口接口类
 * 
 * 为SerialInterface提供Qt信号槽机制支持
 */
class QtSerialInterface : public QObject {
    Q_OBJECT
    
public:
    explicit QtSerialInterface(QObject* parent = nullptr);
    ~QtSerialInterface();
    
    // 代理SerialInterface的方法
    bool open(const QString& portName, int baudRate);
    bool open(const QString& portName, const SerialPortConfig& config);
    void close();
    bool isConnected() const { return m_serial->isOpen(); }
    
    QString getCurrentPort() const;
    int getCurrentBaudRate() const;
    
    bool sendCommand(const QString& command);
    QString sendAndReceive(const QString& command, int timeoutMs = 5000);
    
    // 获取可用端口
    static QStringList getAvailablePorts();
    
    // 设置自动重连
    void setAutoReconnect(bool enable);
    
signals:
    void connected();
    void disconnected();
    void lineReceived(const QString& line);
    void dataReceived(const QByteArray& data);
    void errorOccurred(const QString& error);
    
private slots:
    void pollForData();
    
private:
    std::unique_ptr<SerialInterface> m_serial;
    QTimer* m_pollTimer;
    QString m_readBuffer;
    
    void processReceivedData();
};

#endif // QT_SERIAL_INTERFACE_H