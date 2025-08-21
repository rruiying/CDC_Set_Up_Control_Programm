#ifndef ADDDEVICEDIALOG_H
#define ADDDEVICEDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QTimer>
#include <QSerialPortInfo>
#include "../../src/models/include/device_info.h"

class QLineEdit;
class QComboBox;
class QDialogButtonBox;
class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui { class AddDeviceDialog; }
QT_END_NAMESPACE

class AddDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDeviceDialog(QWidget *parent = nullptr, 
                           const QStringList& existingDevices = QStringList(),
                           const QList<DeviceInfo>& connectedDevices = QList<DeviceInfo>());
    
    ~AddDeviceDialog();

    DeviceInfo getDeviceInfo() const;

    static bool hasAvailablePorts();

    static QStringList getAvailablePortNames();

protected:
    void showEvent(QShowEvent* event) override;

private slots:

    void onDeviceNameChanged(const QString& text);

    void onPortSelectionChanged(int index);

    void onBaudRateChanged(int index);

    void refreshPortList();

    void onRefreshTimer();

    void validateInput();

    void accept() override;

    void reject() override;

private:

    void setupConnections();

    void initializePortList();

    void initializeBaudRateList();

    QPair<bool, QString> validateDeviceName(const QString& name) const;

    QPair<bool, QString> validatePortSelection() const;

    bool isPortOccupiedByApp(const QString& portName) const;
    
    QString getDeviceNameByPort(const QString& portName) const;
    void updateOkButtonState();
    void showValidationError(const QString& message);
    void clearValidationError();

    void logUserOperation(const QString& operation);

private:
    Ui::AddDeviceDialog *ui;  
    

    QStringList existingDeviceNames; 
    QList<DeviceInfo> connectedDevices; 
    QStringList availablePorts;       
    QTimer* refreshTimer;     
    

    bool deviceNameValid;
    bool portSelectionValid;
    bool baudRateValid;


    bool isInitialized;
    QString lastValidationError;

    static QStringList cachedPorts;
    static QDateTime lastScanTime;
    static const int CACHE_VALIDITY_MS = 3000;
    static const int PORT_REFRESH_INTERVAL = 2000;
    static const int MAX_DEVICE_NAME_LENGTH = 50;
};

#endif // ADDDEVICEDIALOG_H