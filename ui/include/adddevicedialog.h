#ifndef ADDDEVICEDIALOG_H
#define ADDDEVICEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class AddDeviceDialog; }
QT_END_NAMESPACE

class AddDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    AddDeviceDialog(QWidget *parent = nullptr);
    ~AddDeviceDialog();

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    Ui::AddDeviceDialog *ui;

    QString getDeviceName() const;
    QString getSerialPort() const;
    int getBaudRate() const;
};

#endif // ADDDEVICEDIALOG_H
