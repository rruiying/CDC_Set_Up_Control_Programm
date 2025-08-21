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
    // Set dialog properties
    setModal(true);
    setWindowTitle(dialogTitle);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    
    // Set fixed size
    setFixedSize(400, 150);

    // Set up UI
    setupUI();

    // Center dialog
    centerDialog();
    
    // Log dialog display
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
    QString message = QString("Are you sure you want to delete device '%1'?\n\n"
        "This will disconnect the device and cannot be undone.")
                     .arg(deviceName);
    
    return confirm(parent, message, "Delete Device", "Delete", "Cancel");
}

bool ConfirmDialog::confirmEmergencyStop(QWidget* parent)
{
    QString message = "Are you sure you want to execute emergency stop?\n\n"
    "This will immediately stop all motor movements.";
    
    return confirm(parent, message, "Emergency Stop", "Stop Now", "Cancel");
}

bool ConfirmDialog::confirmClearLog(QWidget* parent)
{
    QString message = "Are you sure you want to clear all log records?\n\n"
    "Cleared logs cannot be recovered.";
    
    return confirm(parent, message, "Clear Log", "Clear", "Cancel");
}

bool ConfirmDialog::confirmResetSettings(QWidget* parent)
{
    QString message = "Are you sure you want to reset all system settings to default?\n\n"
    "Current configuration will be lost.";
    
    return confirm(parent, message, "Reset Settings", "Reset", "Cancel");
}

bool ConfirmDialog::confirmOverwriteFile(QWidget* parent, const QString& fileName)
{
    QString message = QString("File '%1' already exists.\n\n"
        "Do you want to overwrite the existing file?")
                     .arg(fileName);
    
    return confirm(parent, message, "File Overwrite", "Overwrite", "Cancel");
}

bool ConfirmDialog::confirmDisconnectDevice(QWidget* parent, const QString& deviceName)
{
    QString message = QString("Are you sure you want to disconnect device '%1'?\n\n"
        "Communication with the device will be terminated.")
                     .arg(deviceName);

    return confirm(parent, message, "Disconnect Device", "Disconnect", "Cancel");
}

void ConfirmDialog::setupUI()
{
    // Create main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Create content layout (icon + message)
    contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);

    // Create confirmation icon
    iconLabel = new QLabel();
    iconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(32, 32));
    iconLabel->setAlignment(Qt::AlignTop);
    iconLabel->setFixedSize(32, 32);

    // Create confirmation message label
    messageLabel = new QLabel();
    messageLabel->setText(confirmMessage);
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    messageLabel->setStyleSheet("QLabel { color: #333; font-size: 10pt; }");

    // Add to content layout
    contentLayout->addWidget(iconLabel);
    contentLayout->addWidget(messageLabel, 1);

    // Create button layout
    buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // Left stretch

    // Create confirmation button
    confirmButton = new QPushButton(confirmButtonText);
    confirmButton->setDefault(true);
    confirmButton->setFixedSize(75, 25);
    confirmButton->setStyleSheet("QPushButton { min-width: 75px; }");

    // Create cancel button
    cancelButton = new QPushButton(cancelButtonText);
    cancelButton->setFixedSize(75, 25);
    cancelButton->setStyleSheet("QPushButton { min-width: 75px; }");

    // Connect signals
    connect(confirmButton, &QPushButton::clicked, this, &ConfirmDialog::onConfirmClicked);
    connect(cancelButton, &QPushButton::clicked, this, &ConfirmDialog::onCancelClicked);

    // Add buttons to layout
    buttonLayout->addWidget(confirmButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();

    // Add to main layout
    mainLayout->addLayout(contentLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Set layout
    setLayout(mainLayout);

    // Set focus to confirm button
    confirmButton->setFocus();
}

void ConfirmDialog::centerDialog()
{
    if (parentWidget()) {
        
        QPoint parentCenter = parentWidget()->mapToGlobal(parentWidget()->rect().center());
        move(parentCenter.x() - width() / 2, parentCenter.y() - height() / 2);
    } else {
        
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
    accept(); 
}

void ConfirmDialog::onCancelClicked()
{
    userConfirmed = false;
    reject(); 
}

void ConfirmDialog::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            
            onConfirmClicked();
            break;
        case Qt::Key_Escape:
            
            onCancelClicked();
            break;
        default:
            
            QDialog::keyPressEvent(event);
            break;
    }
}