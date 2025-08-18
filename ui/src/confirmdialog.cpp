// ui/src/confirmdialog.cpp
#include "../include/confirmdialog.h"
#include "../../src/utils/include/logger.h"
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QScreen>
#include <QStyle>
#include <QKeyEvent>
#include <QSpacerItem>

ConfirmDialog::ConfirmDialog(QWidget* parent,
                           const QString& message,
                           const QString& title,
                           const QString& confirmText,
                           const QString& cancelText)
    : QDialog(parent)
    , iconLabel(nullptr)
    , messageLabel(nullptr)
    , confirmButton(nullptr)
    , cancelButton(nullptr)
    , mainLayout(nullptr)
    , contentLayout(nullptr)
    , buttonLayout(nullptr)
    , confirmMessage(message)
    , dialogTitle(title)
    , confirmButtonText(confirmText)
    , cancelButtonText(cancelText)
    , userConfirmed(false)
{
    // 设置对话框属性
    setModal(true);
    setWindowTitle(dialogTitle);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    
    // 设置固定大小
    setFixedSize(400, 150);
    
    // 设置UI
    setupUI();
    
    // 居中显示
    centerDialog();
    
    // 记录确认对话框的显示
    Logger::getInstance().info("Confirm dialog shown: " + message.toStdString());
}

bool ConfirmDialog::confirm(QWidget* parent,
                          const QString& message,
                          const QString& title,
                          const QString& confirmText,
                          const QString& cancelText)
{
    // 创建并显示确认对话框
    ConfirmDialog dialog(parent, message, title, confirmText, cancelText);
    int result = dialog.exec();
    
    // 记录用户选择
    Logger& logger = Logger::getInstance();
    if (result == QDialog::Accepted) {
        logger.info("User confirmed: " + message.toStdString());
        return true;
    } else {
        logger.info("User cancelled: " + message.toStdString());
        return false;
    }
}

bool ConfirmDialog::confirmDeleteDevice(QWidget* parent, const QString& deviceName)
{
    QString message = QString("确定要删除设备 '%1' 吗？\n\n删除后将断开与该设备的连接，且无法恢复。")
                     .arg(deviceName);
    
    return confirm(parent, message, "删除设备", "删除", "取消");
}

bool ConfirmDialog::confirmEmergencyStop(QWidget* parent)
{
    QString message = "确定要执行紧急停止吗？\n\n这将立即停止所有电机运动。";
    
    return confirm(parent, message, "紧急停止", "立即停止", "取消");
}

bool ConfirmDialog::confirmClearLog(QWidget* parent)
{
    QString message = "确定要清空所有日志记录吗？\n\n清空后将无法恢复已删除的日志信息。";
    
    return confirm(parent, message, "清空日志", "清空", "取消");
}

bool ConfirmDialog::confirmResetSettings(QWidget* parent)
{
    QString message = "确定要重置所有系统设置到默认值吗？\n\n重置后当前的配置将丢失。";
    
    return confirm(parent, message, "重置设置", "重置", "取消");
}

bool ConfirmDialog::confirmOverwriteFile(QWidget* parent, const QString& fileName)
{
    QString message = QString("文件 '%1' 已存在。\n\n是否要覆盖现有文件？")
                     .arg(fileName);
    
    return confirm(parent, message, "文件覆盖", "覆盖", "取消");
}

bool ConfirmDialog::confirmDisconnectDevice(QWidget* parent, const QString& deviceName)
{
    QString message = QString("确定要断开设备 '%1' 的连接吗？\n\n断开后将无法与该设备通信。")
                     .arg(deviceName);
    
    return confirm(parent, message, "断开连接", "断开", "取消");
}

void ConfirmDialog::setupUI()
{
    // 创建主布局
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    
    // 创建内容布局（图标 + 消息）
    contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);
    
    // 创建确认图标
    iconLabel = new QLabel();
    iconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(32, 32));
    iconLabel->setAlignment(Qt::AlignTop);
    iconLabel->setFixedSize(32, 32);
    
    // 创建确认消息标签
    messageLabel = new QLabel();
    messageLabel->setText(confirmMessage);
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    messageLabel->setStyleSheet("QLabel { color: #333; font-size: 10pt; }");
    
    // 添加到内容布局
    contentLayout->addWidget(iconLabel);
    contentLayout->addWidget(messageLabel, 1);
    
    // 创建按钮布局
    buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // 左侧弹簧
    
    // 创建确认按钮
    confirmButton = new QPushButton(confirmButtonText);
    confirmButton->setDefault(true);
    confirmButton->setFixedSize(75, 25);
    confirmButton->setStyleSheet("QPushButton { min-width: 75px; }");
    
    // 创建取消按钮
    cancelButton = new QPushButton(cancelButtonText);
    cancelButton->setFixedSize(75, 25);
    cancelButton->setStyleSheet("QPushButton { min-width: 75px; }");
    
    // 连接信号
    connect(confirmButton, &QPushButton::clicked, this, &ConfirmDialog::onConfirmClicked);
    connect(cancelButton, &QPushButton::clicked, this, &ConfirmDialog::onCancelClicked);
    
    // 添加按钮到布局
    buttonLayout->addWidget(confirmButton);
    buttonLayout->addSpacing(10); // 按钮间距
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch(); // 右侧弹簧
    
    // 添加到主布局
    mainLayout->addLayout(contentLayout);
    mainLayout->addStretch(); // 中间弹簧
    mainLayout->addLayout(buttonLayout);
    
    // 设置布局
    setLayout(mainLayout);
    
    // 设置焦点到确认按钮
    confirmButton->setFocus();
}

void ConfirmDialog::centerDialog()
{
    if (parentWidget()) {
        // 相对于父窗口居中
        QPoint parentCenter = parentWidget()->mapToGlobal(parentWidget()->rect().center());
        move(parentCenter.x() - width() / 2, parentCenter.y() - height() / 2);
    } else {
        // 相对于屏幕居中
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenGeometry = screen->availableGeometry();
            QPoint screenCenter = screenGeometry.center();
            move(screenCenter.x() - width() / 2, screenCenter.y() - height() / 2);
        }
    }
}

void ConfirmDialog::onConfirmClicked()
{
    userConfirmed = true;
    accept(); // 关闭对话框并返回Accepted结果
}

void ConfirmDialog::onCancelClicked()
{
    userConfirmed = false;
    reject(); // 关闭对话框并返回Rejected结果
}

void ConfirmDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            // 回车键确认
            onConfirmClicked();
            break;
        case Qt::Key_Escape:
            // ESC键取消
            onCancelClicked();
            break;
        default:
            // 其他键传递给父类处理
            QDialog::keyPressEvent(event);
            break;
    }
}