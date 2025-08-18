// ui/include/mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QListWidget>
#include <QListWidgetItem>
#include <QString>
#include <QStringList>
#include <QResizeEvent>
#include <QSplitter>
#include <memory>
#include "../../src/core/include/motor_controller.h"
#include "../../src/core/include/safety_manager.h"
#include "../../src/utils/include/math_utils.h"
#include "../../src/core/include/sensor_manager.h"
#include "../../src/core/include/data_recorder.h"
#include "../../src/data/include/export_manager.h"
#include "../../src/utils/include/logger.h"
#include "data_visualization_widget.h"
#include "chart_3d_widget.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>

// 前向声明
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 项目头文件
class DeviceInfo;
class SerialInterface;

// 在全局作用域声明
Q_DECLARE_METATYPE(SensorData)

/**
 * @brief 主窗口类
 * 
 * CDC控制系统的主界面，集成所有功能模块：
 * - 设备管理（Serial Communication标签页）
 * - 电机控制（Motor Control标签页）
 * - 传感器监控（Sensor Monitor标签页）
 * - 日志查看（Log Viewer标签页）
 * - 数据可视化（Data Visualization标签页）
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    /**
     * @brief 关闭事件处理
     * @param event 关闭事件
     */
    void closeEvent(QCloseEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

private slots:
    // ===== 第一页：设备管理相关槽函数 =====
    
    /**
     * @brief 添加设备按钮点击
     */
    void onAddDeviceClicked();
    
    /**
     * @brief 删除设备按钮点击
     */
    void onRemoveDeviceClicked();
    
    /**
     * @brief 连接设备按钮点击
     */
    void onConnectDeviceClicked();
    
    /**
     * @brief 断开设备按钮点击
     */
    void onDisconnectDeviceClicked();
    
    /**
     * @brief 发送命令按钮点击
     */
    void onSendCommandClicked();
    
    /**
     * @brief 设备列表选择变化
     * @param current 当前选中项
     * @param previous 之前选中项
     */
    void onDeviceSelectionChanged(QListWidgetItem *current, QListWidgetItem *previous);
    
    /**
     * @brief 命令输入框回车键处理
     */
    void onCommandLineReturnPressed();
    
    // ===== 串口通信回调槽函数 =====
    
    /**
     * @brief 串口连接状态变化回调
     * @param connected 是否连接
     */
    void onSerialConnectionChanged(bool connected);
    
    /**
     * @brief 串口数据接收回调
     * @param data 接收到的数据
     */
    void onSerialDataReceived(const QString& data);
    
    /**
     * @brief 串口错误回调
     * @param error 错误信息
     */
    void onSerialError(const QString& error);

    // ===== 第二页：电机控制相关槽函数 =====
    
    /**
     * @brief 设置高度按钮点击
     */
    void onSetHeightClicked();
    
    /**
     * @brief 设置角度按钮点击
     */
    void onSetAngleClicked();
    
    /**
     * @brief 移动到位置按钮点击
     */
    void onMoveToPositionClicked();
    
    /**
     * @brief 回零按钮点击
     */
    void onHomePositionClicked();
    
    /**
     * @brief 停止电机按钮点击
     */
    void onStopMotorClicked();
    
    /**
     * @brief 紧急停止按钮点击
     */
    void onEmergencyStopClicked();
    
    /**
     * @brief 安全限位参数变化
     */
    void onSafetyLimitsChanged();
    
    // ===== 电机控制器回调槽函数 =====
    
    /**
     * @brief 电机状态变化回调
     * @param status 电机状态（转换为int传递）
     */
    void onMotorStatusChanged(int status);
    
    /**
     * @brief 电机错误回调
     * @param error 错误信息
     */
    void onMotorError(const QString& error);

    // ===== 第三页：传感器监控相关槽函数 =====
    
    /**
     * @brief 记录数据按钮点击
     */
    void onRecordDataClicked();
    
    /**
     * @brief 导出数据按钮点击
     */
    void onExportDataClicked();
    
    /**
     * @brief 传感器数据接收回调
     * @param data 传感器数据
     */
    void onSensorDataReceived(const SensorData& data);
    
    /**
     * @brief 传感器错误回调
     * @param error 错误信息
     */
    void onSensorError(const QString& error);
    
    /**
     * @brief 记录数量变化回调
     * @param recordCount 当前记录数
     */
    void onRecordCountChanged(int recordCount);
    
    /**
     * @brief 导出进度回调
     * @param percentage 进度百分比
     */
    void onExportProgress(int percentage);

    // ===== 第四页：日志查看相关槽函数 =====
    
    /**
     * @brief 日志级别变化
     * @param index 下拉框索引
     */
    void onLogLevelChanged(int index);
    
    /**
     * @brief 清空日志按钮点击
     */
    void onClearLogClicked();
    
    /**
     * @brief 保存日志按钮点击
     */
    void onSaveLogClicked();
    
    /**
     * @brief 定时更新日志显示
     */
    void updateLogDisplay();

    // // 第五页槽函数
    // void onChartTypeChanged(int index);
    // void onDataSourceChanged(int index);
    // void onLoadCsvClicked();
    // void onChartSettingsChanged();
    // void onShowGridChanged(bool checked);
    // void onAutoScaleChanged(bool checked);
    // void onChartRangeChanged();
    // void updateChartStatistics();
    // void onChartDataUpdated(int pointCount);

private:
    // ===== 第一页：设备管理相关方法 =====
    
    /**
     * @brief 初始化设备管理UI
     */
    void initializeDeviceManagement();
    
    /**
     * @brief 更新设备列表显示
     */
    void updateDeviceListWidget();
    
    /**
     * @brief 更新选中设备显示
     */
    void updateSelectedDeviceDisplay();
    
    /**
     * @brief 更新设备操作按钮状态
     */
    void updateDeviceButtons();
    
    /**
     * @brief 添加设备到列表
     * @param device 设备信息
     */
    void addDeviceToList(const DeviceInfo& device);
    
    /**
     * @brief 从列表移除设备
     * @param index 设备索引
     */
    void removeDeviceFromList(int index);
    
    /**
     * @brief 获取当前选中的设备
     * @return 设备指针，如果未选中则返回nullptr
     */
    DeviceInfo* getCurrentSelectedDevice();
    
    /**
     * @brief 获取当前选中的设备索引
     * @return 设备索引，如果未选中则返回-1
     */
    int getCurrentSelectedDeviceIndex() const;
    
    /**
     * @brief 获取已存在的设备名称列表
     * @return 设备名称列表
     */
    QStringList getExistingDeviceNames() const;
    
    /**
     * @brief 获取已连接的设备列表
     * @return 连接设备列表
     */
    QList<DeviceInfo> getConnectedDevices() const;
    
    /**
     * @brief 发送命令到当前设备
     * @param command 命令字符串
     */
    void sendCommandToCurrentDevice(const QString& command);
    
    /**
     * @brief 添加通信日志
     * @param message 日志消息
     * @param isOutgoing 是否为发送消息（true=发送，false=接收）
     */
    void addCommunicationLog(const QString& message, bool isOutgoing = false);
    
    /**
     * @brief 清空通信日志
     */
    void clearCommunicationLog();
    
    /**
     * @brief 保存通信日志到文件
     */
    void saveCommunicationLog();
    
    /**
     * @brief 检查设备名称是否已存在
     * @param name 设备名称
     * @return true如果已存在
     */
    bool isDeviceNameExists(const QString& name) const;
    
    /**
     * @brief 检查端口是否已被使用
     * @param portName 端口名称
     * @return true如果已被使用
     */
    bool isPortInUse(const QString& portName) const;
    
    /**
     * @brief 获取设备在列表中的显示文本
     * @param device 设备信息
     * @return 显示文本
     */
    QString getDeviceDisplayText(const DeviceInfo& device) const;

        // ===== 响应式布局相关方法 ===== <-- 在这里添加新的section
    
    /**
     * @brief 设置响应式布局
     * 
     * 为所有页面设置自适应布局，使控件能够随窗口大小变化
     * 在构造函数中setupUi之后调用
     */
    void setupResponsiveLayout();
    
    /**
     * @brief 根据窗口大小调整布局
     * @param size 新的窗口大小
     * 
     * 根据不同的窗口大小切换不同的布局模式：
     * - 小屏幕模式 (<1024px)：紧凑布局
     * - 标准模式 (1024-1600px)：标准布局
     * - 大屏幕模式 (>1600px)：宽松布局
     */
    void adjustLayoutForSize(const QSize &size);
    
    /**
     * @brief 设置紧凑模式
     * @param enable 是否启用紧凑模式
     * 
     * 在小屏幕时使用，隐藏次要信息，减小间距
     */
    void setCompactMode(bool enable);
    
    /**
     * @brief 设置标准模式
     * @param enable 是否启用标准模式
     * 
     * 默认布局模式，适合大多数屏幕
     */
    void setStandardMode(bool enable);
    
    /**
     * @brief 设置宽屏模式
     * @param enable 是否启用宽屏模式
     * 
     * 在大屏幕时使用，增大间距和字体
     */
    void setWideScreenMode(bool enable);
    
    /**
     * @brief 调整UI缩放
     * @param width 窗口宽度
     * @param height 窗口高度
     * 
     * 根据窗口大小动态调整字体、图标和间距
     */
    void adjustUIScale(int width, int height);
    
    /**
     * @brief 为设备管理页设置响应式布局
     */
    void setupDeviceManagementResponsiveLayout();
    
    /**
     * @brief 为电机控制页设置响应式布局
     */
    void setupMotorControlResponsiveLayout();
    
    /**
     * @brief 为传感器监控页设置响应式布局
     */
    void setupSensorMonitorResponsiveLayout();
    
    /**
     * @brief 为日志查看页设置响应式布局
     */
    void setupLogViewerResponsiveLayout();
    
    /**
     * @brief 保存窗口状态
     * 
     * 保存窗口大小、位置和分割器状态到设置
     */
    void saveWindowState();
    
    /**
     * @brief 恢复窗口状态
     * 
     * 从设置中恢复上次的窗口状态
     */
    void restoreWindowState();
    
    // ===== 通用辅助方法 =====
    
    /**
     * @brief 记录用户操作到日志
     * @param operation 操作描述
     */
    void logUserOperation(const QString& operation);
    
    /**
     * @brief 显示状态栏消息
     * @param message 消息内容
     * @param timeout 显示时间（毫秒）
     */
    void showStatusMessage(const QString& message, int timeout = 3000);
    
    // ===== 第二页：电机控制相关方法 =====
    
    /**
     * @brief 初始化电机控制UI
     */
    void initializeMotorControl();
    
    /**
     * @brief 设置电机控制UI的初始状态
     */
    void setupMotorControlUI();
    
    /**
     * @brief 更新电机控制显示
     */
    void updateMotorControlDisplay();
    
    /**
     * @brief 更新电机控制按钮状态
     */
    void updateMotorControlButtons();
    
    /**
     * @brief 检查安全限位
     * @param height 目标高度
     * @param angle 目标角度
     * @return true 如果在安全范围内
     */
    bool checkSafetyLimits(double height, double angle);
    
    /**
     * @brief 更新安全管理器的限位设置
     */
    void updateSafetyManagerLimits();
    
    /**
     * @brief 更新目标位置SpinBox的范围
     */
    void updateTargetPositionRanges();
    
    /**
     * @brief 更新理论电容计算
     */
    void updateTheoreticalCapacitance();
    
    /**
     * @brief 创建电机控制器
     */
    void createMotorController();

    // ===== 第三页：传感器监控相关方法 =====
    
    /**
     * @brief 初始化传感器监控UI
     */
    void initializeSensorMonitor();
    
    /**
     * @brief 创建传感器管理器
     */
    void createSensorManager();
    
    /**
     * @brief 更新传感器监控显示
     */
    void updateSensorMonitorDisplay();
    
    /**
     * @brief 计算理论电容值
     * @return 理论电容值(pF)
     */
    double calculateTheoreticalCapacitance() const;
    
    /**
     * @brief 启动传感器监控
     */
    void startSensorMonitoring();
    
    /**
     * @brief 停止传感器监控
     */
    void stopSensorMonitoring();

    // ===== 第四页：日志查看相关方法 =====
    
    /**
     * @brief 初始化日志查看UI
     */
    void initializeLogViewer();
    
    /**
     * @brief 格式化日志条目
     * @param entry 日志条目
     * @return 格式化的字符串
     */
    QString formatLogEntry(const LogEntry& entry) const;
    
    /**
     * @brief 格式化时间戳
     * @param time 时间点
     * @return 格式化的时间字符串
     */
    QString formatLogTimestamp(const std::chrono::system_clock::time_point& time) const;
    
    /**
     * @brief 添加日志到显示区域
     * @param logLine 日志行
     * @param level 日志级别（用于颜色）
     */
    void appendLogToDisplay(const QString& logLine, LogLevel level);
    
    /**
     * @brief 生成日志文件名
     * @return 默认文件名
     */
    QString generateLogFilename() const;
    
    /**
     * @brief 保存日志到文件
     * @param logs 日志条目列表
     * @param filename 文件名
     * @return 是否成功
     */
    bool saveLogsToFile(const std::vector<LogEntry>& logs, const QString& filename) const;
    
    /**
     * @brief 强制更新日志显示
     */
    void forceLogUpdate();

    // ===== 增强的日志方法 =====
    void logError(const QString& error, const QString& context = "");
    void logWarning(const QString& warning, const QString& context = "");
    void logInfo(const QString& info, const QString& context = "");

    // // ===== 第五页：数据可视化方法 =====
    // void initializeDataVisualization();
    // void setupDataVisualizationUI();
    // void connectDataVisualizationSignals();

private:
    // UI对象
    Ui::MainWindow *ui;

    // ===== 响应式布局相关成员变量 ===== <-- 在成员变量section添加
    
    // 布局相关
    QSplitter* m_devicePageSplitter;        ///< 设备管理页分割器
    QSplitter* m_sensorPageSplitter;        ///< 传感器页分割器
    
    // 布局模式
    enum class LayoutMode {
        Compact,    ///< 紧凑模式 (宽度 < 1024)
        Standard,   ///< 标准模式 (1024 <= 宽度 < 1600)
        Wide        ///< 宽屏模式 (宽度 >= 1600)
    };
    LayoutMode m_currentLayoutMode;         ///< 当前布局模式
    
    // 窗口状态
    QSize m_lastWindowSize;                 ///< 上次窗口大小
    bool m_isResponsiveLayoutEnabled;       ///< 是否启用响应式布局
    
    // ===== 第一页：设备管理相关成员变量 =====
    
    QList<DeviceInfo> deviceList;              ///< 设备列表
    int currentSelectedDeviceIndex;             ///< 当前选中设备索引
    std::shared_ptr<SerialInterface> serialInterface;  ///< 串口接口

    // ===== 第二页：电机控制相关成员变量 =====
    
    std::shared_ptr<MotorController> motorController;    ///< 电机控制器
    std::shared_ptr<SafetyManager> safetyManager;        ///< 安全管理器
    
    // 位置和状态
    double targetHeight;                    ///< 目标高度(mm)
    double targetAngle;                     ///< 目标角度(度)
    double currentHeight;                   ///< 当前高度(mm) - 显示用
    double currentAngle;                    ///< 当前角度(度) - 显示用
    double theoreticalCapacitance;          ///< 理论电容值(pF)

    // ===== 第三页：传感器监控相关成员变量 =====
    
    std::shared_ptr<SensorManager> sensorManager;      ///< 传感器管理器
    std::shared_ptr<DataRecorder> dataRecorder;        ///< 数据记录器  
    std::shared_ptr<ExportManager> exportManager;      ///< 导出管理器
    
    // 当前传感器数据
    SensorData currentSensorData;                       ///< 当前传感器数据
    bool hasValidSensorData;                            ///< 是否有有效数据
    QDateTime lastRecordTime;                           ///< 最后记录时间

    // UI状态标志
    bool isInitialized;                         ///< 是否已初始化

    // ===== 第四页：日志查看相关成员变量 =====
    
    QTimer* logUpdateTimer;                     ///< 日志更新定时器
    LogLevel currentLogLevel;                   ///< 当前选择的日志级别
    size_t lastDisplayedLogCount;
    
    // ===== 第五页：数据可视化 =====
    DataVisualizationWidget* dataVisualizationWidget;
    ChartDisplayWidget* chartDisplayWidget;
    std::unique_ptr<CsvAnalyzer> csvAnalyzer;
    Chart3DWidget* chart3DWidget; 
        
    // 当前图表设置
    ChartType currentChartType;
    DataSource currentDataSource;
    bool isLoadingData;
        
    // 固定条件（用于2D图表）
    struct {
        double fixedHeight = 50.0;
        double fixedAngle = 0.0;
        double fixedTemperature = 25.0;
    } chartFixedConditions;///< 上次显示的日志数量
    
    // 常量
    static const int MAX_LOG_DISPLAY_LINES = 10000;  ///< 最大显示行数
    
    // 常量
    static const int MAX_COMMUNICATION_LOG_LINES = 1000;  ///< 通信日志最大行数
    static const int MIN_WINDOW_WIDTH = 800;     ///< 最小窗口宽度
    static const int MIN_WINDOW_HEIGHT = 600;    ///< 最小窗口高度
    static const int COMPACT_MODE_THRESHOLD = 1024;   ///< 紧凑模式阈值
    static const int WIDE_MODE_THRESHOLD = 1600;      ///< 宽屏模式阈值
};

#endif // MAINWINDOW_H