#include "ui/include/adddevicedialog.h"
#include "ui_adddevicedialog.h"
#include <QSerialPortInfo>

AddDeviceDialog::AddDeviceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddDeviceDialog)
{
    ui->setupUi(this);
    
    setWindowTitle("Add Serial Device");
    
    // 填充可用端口
    updatePortList();
    
    // 设置默认波特率
    ui->comboBox_2->setCurrentText("115200");
}

AddDeviceDialog::~AddDeviceDialog()
{
    delete ui;
}

QString AddDeviceDialog::getDeviceName() const
{
    return ui->lineEdit->text();
}

QString AddDeviceDialog::getPortName() const
{
    return ui->comboBox->currentText();
}

int AddDeviceDialog::getBaudRate() const
{
    return ui->comboBox_2->currentText().toInt();
}

void AddDeviceDialog::updatePortList()
{
    ui->comboBox->clear();
    
    // 获取所有可用的串口
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        QString description = port.description();
        if (description.isEmpty()) {
            description = "Serial Port";
        }
        
        QString displayName = QString("%1 (%2)")
            .arg(port.portName())
            .arg(description);
            
        ui->comboBox->addItem(port.portName());
    }
    
    // 如果没有找到串口
    if (ui->comboBox->count() == 0) {
        ui->comboBox->addItem("No ports available");
    }
}

void AddDeviceDialog::on_buttonBox_accepted()
{
    // 验证输入
    if (ui->lineEdit->text().isEmpty()) {
        ui->lineEdit->setText(ui->comboBox->currentText());
    }
    
    accept();
}

void AddDeviceDialog::on_buttonBox_rejected()
{
    reject();
}