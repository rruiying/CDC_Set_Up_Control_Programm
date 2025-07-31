#pragma once

#include <QDialog>

namespace Ui {
class AddDeviceDialog;
}

class AddDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDeviceDialog(QWidget *parent = nullptr);
    ~AddDeviceDialog();

    // 获取用户输入
    QString getDeviceName() const;
    QString getPortName() const;
    int getBaudRate() const;

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::AddDeviceDialog *ui;
    
    void updatePortList();
};