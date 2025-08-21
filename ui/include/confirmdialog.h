#ifndef CONFIRMDIALOG_H
#define CONFIRMDIALOG_H

#include <QDialog>
#include <QString>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>


class ConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    static bool confirm(QWidget* parent, 
                       const QString& message,
                       const QString& title = "Confirm",
                       const QString& confirmText = "OK",
                       const QString& cancelText = "Cancel");

    static bool confirmDeleteDevice(QWidget* parent, const QString& deviceName);

    static bool confirmEmergencyStop(QWidget* parent);

    static bool confirmClearLog(QWidget* parent);

    static bool confirmResetSettings(QWidget* parent);


    static bool confirmOverwriteFile(QWidget* parent, const QString& fileName);


    static bool confirmDisconnectDevice(QWidget* parent, const QString& deviceName);

private:

    explicit ConfirmDialog(QWidget* parent,
                          const QString& message,
                          const QString& title,
                          const QString& confirmText,
                          const QString& cancelText);


    void setupUI();


    void centerDialog();

private slots:

    void onConfirmClicked();

    void onCancelClicked();

protected:

    void keyPressEvent(QKeyEvent* event) override;

private:

    QLabel* iconLabel;              
    QLabel* messageLabel;           
    QPushButton* confirmButton;     
    QPushButton* cancelButton;      
    QVBoxLayout* mainLayout;        
    QHBoxLayout* contentLayout;     
    QHBoxLayout* buttonLayout;      

    QString confirmMessage;         
    QString dialogTitle;           
    QString confirmButtonText;      
    QString cancelButtonText;       

    bool userConfirmed;             
};

#endif // CONFIRMDIALOG_H