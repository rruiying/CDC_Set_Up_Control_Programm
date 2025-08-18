// ui/src/errordialog.cpp
#include "../include/errordialog.h"
#include "../../src/utils/include/logger.h"
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QStyle>
#include <QMessageBox>
#include <QSpacerItem>

ErrorDialog::ErrorDialog(QWidget* parent, ErrorType type, const QString& message)
    : QDialog(parent)
    , iconLabel(nullptr)
    , messageLabel(nullptr)
    , okButton(nullptr)
    , mainLayout(nullptr)
    , contentLayout(nullptr)
    , buttonLayout(nullptr)
    , errorType(type)
    , errorMessage(message)
{
    // 记录错误到日志系统
    logError(type, message);
    
    // 设置对话框属性
    setModal(true);
    setWindowTitle(getErrorTitle(type));
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    
    // 设置固定大小
    setFixedSize(400, 150);
    
    // 设置UI
    setupUI();
    
    // 居中显示
    centerDialog();
}

void ErrorDialog::showError(QWidget* parent, ErrorType type, const QString& message)
{
    // 创建并显示错误对话框
    ErrorDialog dialog(parent, type, message);
    dialog.exec();
}

QString ErrorDialog::getErrorTypeString(ErrorType type)
{
    switch (type) {
        case CommunicationError:
            return "Communication Error";
        case SafetyLimitError:
            return "Safety Limit Error";
        case FileOperationError:
            return "File Operation Error";
        case HardwareError:
            return "Hardware Error";
        case DataValidationError:
            return "Data Validation Error";
        default:
            return "Unknown Error";
    }
}

void ErrorDialog::setupUI()
{
    // 创建主布局
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    
    // 创建内容布局（图标 + 消息）
    contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);
    
    // 创建错误图标
    iconLabel = new QLabel();
    iconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(32, 32));
    iconLabel->setAlignment(Qt::AlignTop);
    iconLabel->setFixedSize(32, 32);
    
    // 创建错误消息标签
    messageLabel = new QLabel();
    messageLabel->setText(getErrorMessage(errorType, errorMessage));
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    messageLabel->setStyleSheet("QLabel { color: #333; font-size: 10pt; }");
    
    // 添加到内容布局
    contentLayout->addWidget(iconLabel);
    contentLayout->addWidget(messageLabel, 1);
    
    // 创建按钮布局
    buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // 左侧弹簧
    
    // 创建确定按钮
    okButton = new QPushButton("确定");
    okButton->setDefault(true);
    okButton->setFixedSize(75, 25);
    okButton->setStyleSheet("QPushButton { min-width: 75px; }");
    
    // 连接信号
    connect(okButton, &QPushButton::clicked, this, &ErrorDialog::onOkClicked);
    
    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch(); // 右侧弹簧
    
    // 添加到主布局
    mainLayout->addLayout(contentLayout);
    mainLayout->addStretch(); // 中间弹簧
    mainLayout->addLayout(buttonLayout);
    
    // 设置布局
    setLayout(mainLayout);
}

QString ErrorDialog::getErrorTitle(ErrorType type) const
{
    switch (type) {
        case CommunicationError:
            return "通信错误";
        case SafetyLimitError:
            return "安全限位错误";
        case FileOperationError:
            return "文件操作错误";
        case HardwareError:
            return "硬件错误";
        case DataValidationError:
            return "数据验证错误";
        default:
            return "系统错误";
    }
}

QString ErrorDialog::getErrorMessage(ErrorType type, const QString& detail) const
{
    QString baseMessage;
    
    switch (type) {
        case CommunicationError:
            baseMessage = "设备通信出现问题。请检查设备连接状态和串口设置。";
            break;
        case SafetyLimitError:
            baseMessage = "操作超出安全限位范围。请检查设定值是否在允许范围内。";
            break;
        case FileOperationError:
            baseMessage = "文件操作失败。请检查文件路径和访问权限。";
            break;
        case HardwareError:
            baseMessage = "硬件设备出现故障。请检查设备连接和电源状态。";
            break;
        case DataValidationError:
            baseMessage = "输入数据格式不正确。请检查输入值的格式和范围。";
            break;
        default:
            baseMessage = "系统出现未知错误。";
            break;
    }
    
    // 如果有详细信息且不为空，添加提示查看日志
    if (!detail.isEmpty()) {
        baseMessage += "\n\n详细错误信息请查看日志记录。";
    }
    
    return baseMessage;
}

QIcon ErrorDialog::getErrorIcon(ErrorType type) const
{
    // 根据错误类型返回不同图标（目前都使用critical图标）
    switch (type) {
        case CommunicationError:
        case HardwareError:
            return style()->standardIcon(QStyle::SP_MessageBoxCritical);
        case SafetyLimitError:
            return style()->standardIcon(QStyle::SP_MessageBoxWarning);
        case FileOperationError:
        case DataValidationError:
            return style()->standardIcon(QStyle::SP_MessageBoxInformation);
        default:
            return style()->standardIcon(QStyle::SP_MessageBoxCritical);
    }
}

void ErrorDialog::logError(ErrorType type, const QString& message)
{
    Logger& logger = Logger::getInstance();
    
    // 构造详细的日志消息
    QString logMessage = QString("[%1] %2")
                          .arg(getErrorTypeString(type))
                          .arg(message.isEmpty() ? "Error occurred" : message);
    
    // 根据错误类型选择日志级别
    switch (type) {
        case CommunicationError:
        case HardwareError:
            logger.error(logMessage.toStdString());
            break;
        case SafetyLimitError:
            logger.warning(logMessage.toStdString());
            break;
        case FileOperationError:
        case DataValidationError:
            logger.info(logMessage.toStdString());
            break;
        default:
            logger.error(logMessage.toStdString());
            break;
    }
}

void ErrorDialog::centerDialog()
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

void ErrorDialog::onOkClicked()
{
    accept(); // 关闭对话框并返回Accepted结果
}