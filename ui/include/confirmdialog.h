// ui/include/confirmdialog.h
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

/**
 * @brief 确认对话框类
 * 
 * 提供统一的确认机制：
 * - 模态对话框，阻塞主窗口
 * - 自动居中显示  
 * - 原生Windows风格
 * - 可自定义消息和按钮文本
 * - 返回用户选择结果（true=确认，false=取消）
 * - 预定义常用确认场景
 */
class ConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 显示确认对话框（静态方法）
     * @param parent 父窗口指针，可以为nullptr
     * @param message 确认消息内容
     * @param title 对话框标题（默认为"确认操作"）
     * @param confirmText 确认按钮文本（默认为"确认"）
     * @param cancelText 取消按钮文本（默认为"取消"）
     * @return true=用户点击确认，false=用户点击取消或ESC
     * 
     * 这是主要的使用接口，会自动：
     * 1. 创建确认对话框
     * 2. 模态显示并自动居中
     * 3. 返回用户的选择结果
     */
    static bool confirm(QWidget* parent, 
                       const QString& message,
                       const QString& title = "确认操作",
                       const QString& confirmText = "确认",
                       const QString& cancelText = "取消");

    /**
     * @brief 确认删除设备
     * @param parent 父窗口
     * @param deviceName 设备名称
     * @return true=确认删除，false=取消删除
     */
    static bool confirmDeleteDevice(QWidget* parent, const QString& deviceName);

    /**
     * @brief 确认紧急停止
     * @param parent 父窗口
     * @return true=确认停止，false=取消停止
     */
    static bool confirmEmergencyStop(QWidget* parent);

    /**
     * @brief 确认清空日志
     * @param parent 父窗口
     * @return true=确认清空，false=取消清空
     */
    static bool confirmClearLog(QWidget* parent);

    /**
     * @brief 确认重置系统设置
     * @param parent 父窗口
     * @return true=确认重置，false=取消重置
     */
    static bool confirmResetSettings(QWidget* parent);

    /**
     * @brief 确认覆盖文件
     * @param parent 父窗口
     * @param fileName 文件名
     * @return true=确认覆盖，false=取消覆盖
     */
    static bool confirmOverwriteFile(QWidget* parent, const QString& fileName);

    /**
     * @brief 确认断开设备连接
     * @param parent 父窗口
     * @param deviceName 设备名称
     * @return true=确认断开，false=取消断开
     */
    static bool confirmDisconnectDevice(QWidget* parent, const QString& deviceName);

private:
    /**
     * @brief 私有构造函数
     * @param parent 父窗口
     * @param message 确认消息
     * @param title 对话框标题
     * @param confirmText 确认按钮文本
     * @param cancelText 取消按钮文本
     */
    explicit ConfirmDialog(QWidget* parent,
                          const QString& message,
                          const QString& title,
                          const QString& confirmText,
                          const QString& cancelText);

    /**
     * @brief 设置UI界面
     */
    void setupUI();

    /**
     * @brief 居中显示对话框
     */
    void centerDialog();

private slots:
    /**
     * @brief 确认按钮点击处理
     */
    void onConfirmClicked();

    /**
     * @brief 取消按钮点击处理
     */
    void onCancelClicked();

protected:
    /**
     * @brief 重写键盘事件处理（ESC键取消）
     * @param event 键盘事件
     */
    void keyPressEvent(QKeyEvent* event) override;

private:
    // UI组件
    QLabel* iconLabel;              ///< 确认图标标签
    QLabel* messageLabel;           ///< 确认消息标签
    QPushButton* confirmButton;     ///< 确认按钮
    QPushButton* cancelButton;      ///< 取消按钮
    QVBoxLayout* mainLayout;        ///< 主布局
    QHBoxLayout* contentLayout;     ///< 内容布局
    QHBoxLayout* buttonLayout;      ///< 按钮布局

    // 数据成员
    QString confirmMessage;         ///< 确认消息
    QString dialogTitle;            ///< 对话框标题
    QString confirmButtonText;      ///< 确认按钮文本
    QString cancelButtonText;       ///< 取消按钮文本
    
    bool userConfirmed;             ///< 用户是否确认
};

#endif // CONFIRMDIALOG_H