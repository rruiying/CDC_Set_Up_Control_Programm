// ui/include/adddevicedialog.h
#ifndef ADDDEVICEDIALOG_H
#define ADDDEVICEDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QTimer>
#include <QSerialPortInfo>

// 前向声明
class QLineEdit;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class DeviceInfo;

QT_BEGIN_NAMESPACE
namespace Ui { class AddDeviceDialog; }
QT_END_NAMESPACE

/**
 * @brief 添加设备对话框类
 * 
 * 提供设备添加功能：
 * - 自动扫描可用串口
 * - 设备命名和验证
 * - 波特率选择
 * - 端口可用性检查
 * - 数据验证和错误处理
 * - 与现有设备列表的冲突检查
 */
class AddDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     * @param existingDevices 已存在的设备名称列表
     * @param connectedDevices 已连接的设备列表（用于端口占用检查）
     */
    explicit AddDeviceDialog(QWidget *parent = nullptr, 
                           const QStringList& existingDevices = QStringList(),
                           const QList<DeviceInfo>& connectedDevices = QList<DeviceInfo>());
    
    /**
     * @brief 析构函数
     */
    ~AddDeviceDialog();

    /**
     * @brief 获取用户配置的设备信息
     * @return DeviceInfo对象，包含设备名称、端口、波特率等信息
     */
    DeviceInfo getDeviceInfo() const;

    /**
     * @brief 检查是否有可用的串口
     * @return true如果有可用端口
     */
    static bool hasAvailablePorts();

    /**
     * @brief 获取所有可用的串口名称
     * @return 可用串口名称列表
     */
    static QStringList getAvailablePortNames();

protected:
    /**
     * @brief 显示事件处理（用于初始化扫描）
     * @param event 显示事件
     */
    void showEvent(QShowEvent* event) override;

private slots:
    /**
     * @brief 设备名称文本变化处理
     * @param text 新的设备名称
     */
    void onDeviceNameChanged(const QString& text);

    /**
     * @brief 串口选择变化处理
     * @param index 选中的端口索引
     */
    void onPortSelectionChanged(int index);

    /**
     * @brief 波特率选择变化处理
     * @param index 选中的波特率索引
     */
    void onBaudRateChanged(int index);

    /**
     * @brief 刷新端口列表
     */
    void refreshPortList();

    /**
     * @brief 定时刷新端口列表
     */
    void onRefreshTimer();

    /**
     * @brief 验证输入数据
     */
    void validateInput();

    /**
     * @brief 对话框接受处理
     */
    void accept() override;

    /**
     * @brief 对话框拒绝处理
     */
    void reject() override;

private:
    /**
     * @brief 初始化UI连接
     */
    void setupConnections();

    /**
     * @brief 初始化端口列表
     */
    void initializePortList();

    /**
     * @brief 初始化波特率列表
     */
    void initializeBaudRateList();

    /**
     * @brief 验证设备名称
     * @param name 设备名称
     * @return 验证结果和错误消息
     */
    QPair<bool, QString> validateDeviceName(const QString& name) const;

    /**
     * @brief 验证端口选择
     * @return 验证结果和错误消息
     */
    QPair<bool, QString> validatePortSelection() const;

    /**
     * @brief 检查端口是否被当前应用占用
     * @param portName 端口名称
     * @return true如果端口被占用
     */
    bool isPortOccupiedByApp(const QString& portName) const;
    
    /**
     * @brief 获取占用指定端口的设备名称
     * @param portName 端口名称
     * @return 设备名称，如果未被占用则返回空字符串
     */
    QString getDeviceNameByPort(const QString& portName) const;

    /**
     * @brief 更新OK按钮状态
     */
    void updateOkButtonState();

    /**
     * @brief 显示验证错误
     * @param message 错误消息
     */
    void showValidationError(const QString& message);

    /**
     * @brief 清除验证错误显示
     */
    void clearValidationError();

    /**
     * @brief 记录用户操作到日志
     * @param operation 操作描述
     */
    void logUserOperation(const QString& operation);

private:
    Ui::AddDeviceDialog *ui;        ///< UI界面指针
    
    // 数据成员
    QStringList existingDeviceNames;    ///< 已存在的设备名称列表
    QList<DeviceInfo> connectedDevices; ///< 已连接的设备列表
    QStringList availablePorts;         ///< 可用端口列表
    QTimer* refreshTimer;               ///< 端口刷新定时器
    
    // 验证状态
    bool deviceNameValid;               ///< 设备名称是否有效
    bool portSelectionValid;            ///< 端口选择是否有效
    bool baudRateValid;                 ///< 波特率选择是否有效
    
    // UI状态
    bool isInitialized;                 ///< 是否已初始化
    QString lastValidationError;        ///< 最后的验证错误消息
    
    // 常量
    static const int PORT_REFRESH_INTERVAL = 2000;  ///< 端口刷新间隔(ms)
    static const int MAX_DEVICE_NAME_LENGTH = 50;   ///< 最大设备名称长度
};

#endif // ADDDEVICEDIALOG_H