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

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);
    
    iconLabel = new QLabel();
    iconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(32, 32));
    iconLabel->setAlignment(Qt::AlignTop);
    iconLabel->setFixedSize(32, 32);

    messageLabel = new QLabel();
    messageLabel->setText(getErrorMessage(errorType, errorMessage));
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    messageLabel->setStyleSheet("QLabel { color: #333; font-size: 10pt; }");
    

    contentLayout->addWidget(iconLabel);
    contentLayout->addWidget(messageLabel, 1);

    buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // 左侧弹簧
    

    okButton = new QPushButton("确定");
    okButton->setDefault(true);
    okButton->setFixedSize(75, 25);
    okButton->setStyleSheet("QPushButton { min-width: 75px; }");
    

    connect(okButton, &QPushButton::clicked, this, &ErrorDialog::onOkClicked);
    
    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch(); 
    

    mainLayout->addLayout(contentLayout);
    mainLayout->addStretch(); 
    mainLayout->addLayout(buttonLayout);
    

    setLayout(mainLayout);
}

QString ErrorDialog::getErrorTitle(ErrorType type) const
{
    switch (type) {
        case CommunicationError:
            return "communication error";
        case SafetyLimitError:
            return "safety limit error";
        case FileOperationError:
            return "file operation error";
        case HardwareError:
            return "hardware error";
        case DataValidationError:
            return "data validation error";
        default:
            return "system error";
    }
}

QString ErrorDialog::getErrorMessage(ErrorType type, const QString& detail) const
{
    QString baseMessage;
    
    switch (type) {
        case CommunicationError:
            baseMessage = "Device communication error. Please check the device connection status and serial port settings.";
            break;
        case SafetyLimitError:
            baseMessage = "Operation exceeded safety limit. Please check if the set value is within the allowed range.";
            break;
        case FileOperationError:
            baseMessage = "File operation failed. Please check the file path and access permissions.";
            break;
        case HardwareError:
            baseMessage = "Hardware device error. Please check the device connection and power status.";
            break;
        case DataValidationError:
            baseMessage = "Input data format is incorrect. Please check the format and range of the input values.";
            break;
        default:
            baseMessage = "Unknown system error.";
            break;
    }

    // If there is detailed information and it is not empty, add a prompt to check the logs
    if (!detail.isEmpty()) {
        baseMessage += "\n\nFor detailed error information, please check the log records.";
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