#include "adddevicedialog.h"
#include "ui_adddevicedialog.h"

AddDeviceDialog::AddDeviceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddDeviceDialog)
{
    ui->setupUi(this);

    // 连接按钮信号
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddDeviceDialog::onConfirmClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AddDeviceDialog::onCancelClicked);

    setWindowTitle("添加设备");
}

AddDeviceDialog::~AddDeviceDialog()
{
    delete ui;
}

void AddDeviceDialog::onConfirmClicked()
{
    accept();
}

void AddDeviceDialog::onCancelClicked()
{
    reject();
}

QString AddDeviceDialog::getDeviceName() const
{
    return ui->lineEdit->text();
}

QString AddDeviceDialog::getSerialPort() const
{
    return ui->comboBox->currentText();
}

int AddDeviceDialog::getBaudRate() const
{
    return ui->comboBox_2->currentText().toInt();
}
