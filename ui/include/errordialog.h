#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QDialog>
#include <QString>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>

class ErrorDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 错误类型枚举
     */
    enum ErrorType {
        CommunicationError,    ///< 通信错误（超时、连接失败等）
        SafetyLimitError,      ///< 安全限位错误
        FileOperationError,    ///< 文件操作错误  
        HardwareError,         ///< 硬件错误
        DataValidationError    ///< 数据验证错误
    };

    /**
     * @brief 显示错误对话框（静态方法）
     * @param parent 父窗口指针，可以为nullptr
     * @param type 错误类型
     * @param message 错误详细信息（可选），将记录到日志中
     * 
     * 这是主要的使用接口，会自动：
     * 1. 创建适当的错误对话框
     * 2. 将详细错误信息记录到Logger
     * 3. 显示用户友好的错误消息
     * 4. 模态显示并自动居中
     */
    static void showError(QWidget* parent, ErrorType type, const QString& message = "");

    /**
     * @brief 获取错误类型的字符串表示（用于日志）
     * @param type 错误类型
     * @return 错误类型字符串
     */
    static QString getErrorTypeString(ErrorType type);

private:
    /**
     * @brief 私有构造函数
     * @param parent 父窗口
     * @param type 错误类型
     * @param message 错误消息
     */
    explicit ErrorDialog(QWidget* parent, ErrorType type, const QString& message);

    /**
     * @brief 设置UI界面
     */
    void setupUI();

    /**
     * @brief 根据错误类型获取对话框标题
     * @param type 错误类型
     * @return 对话框标题
     */
    QString getErrorTitle(ErrorType type) const;

    /**
     * @brief 根据错误类型和详细信息生成用户友好的错误消息
     * @param type 错误类型
     * @param detail 详细信息
     * @return 用户友好的错误消息
     */
    QString getErrorMessage(ErrorType type, const QString& detail) const;

    /**
     * @brief 获取错误类型对应的图标
     * @param type 错误类型
     * @return 错误图标
     */
    QIcon getErrorIcon(ErrorType type) const;

    /**
     * @brief 记录错误到日志系统
     * @param type 错误类型
     * @param message 错误消息
     */
    void logError(ErrorType type, const QString& message);

    /**
     * @brief 居中显示对话框
     */
    void centerDialog();

private slots:
    /**
     * @brief 确定按钮点击处理
     */
    void onOkClicked();

private:
    // UI组件
    QLabel* iconLabel;           ///< 错误图标标签
    QLabel* messageLabel;        ///< 错误消息标签
    QPushButton* okButton;       ///< 确定按钮
    QVBoxLayout* mainLayout;     ///< 主布局
    QHBoxLayout* contentLayout;  ///< 内容布局
    QHBoxLayout* buttonLayout;   ///< 按钮布局

    // 数据成员
    ErrorType errorType;         ///< 错误类型
    QString errorMessage;        ///< 错误消息
};

#endif // ERRORDIALOG_H