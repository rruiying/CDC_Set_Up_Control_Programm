#include "app/include/application.h"
#include "hardware/include/serial_interface.h"
#include "hardware/include/motor_interface.h"
#include "hardware/include/sensor_interface.h"
#include "core/include/motor_controller.h"
#include "core/include/sensor_manager.h"
#include "core/include/safety_manager.h"
#include "core/include/calibration_engine.h"
#include "core/include/data_recorder.h"
#include "data/include/data_processor.h"
#include "data/include/export_manager.h"
#include "data/include/file_manager.h"
#include "ui/include/mainwindow.h"
#include "utils/include/logger.h"
#include "utils/include/config_manager.h"

Application::Application(QObject* parent)
    : QObject(parent)
    , m_isRunning(false)
{
}

Application::~Application() {
    if (m_isRunning) {
        shutdown();
    }
}

bool Application::initialize() {
    LOG_INFO("Initializing CDC Control Application...");
    
    // 加载配置
    ConfigManager::instance().loadFromFile("config.json");
    
    // 初始化各层
    if (!initializeHardware()) {
        LOG_ERROR("Failed to initialize hardware layer");
        return false;
    }
    
    if (!initializeCore()) {
        LOG_ERROR("Failed to initialize core layer");
        return false;
    }
    
    if (!initializeData()) {
        LOG_ERROR("Failed to initialize data layer");
        return false;
    }
    
    if (!initializeUI()) {
        LOG_ERROR("Failed to initialize UI");
        return false;
    }
    
    // 连接信号
    connectSignals();
    
    m_isRunning = true;
    LOG_INFO("Application initialized successfully");
    emit initialized();
    
    return true;
}

void Application::shutdown() {
    LOG_INFO("Shutting down application...");
    
    // 停止所有活动
    if (m_sensorManager) {
        m_sensorManager->stopMonitoring();
    }
    
    if (m_motorController) {
        m_motorController->stop();
    }
    
    if (m_dataRecorder && m_dataRecorder->isRecording()) {
        m_dataRecorder->stopRecording();
    }
    
    // 断开串口
    if (m_serialInterface && m_serialInterface->isConnected()) {
        m_serialInterface->disconnect();
    }
    
    // 保存配置
    ConfigManager::instance().saveToFile("config.json");
    
    m_isRunning = false;
    LOG_INFO("Application shutdown complete");
}

void Application::showMainWindow() {
    if (m_mainWindow) {
        m_mainWindow->show();
    }
}

void Application::onEmergencyStop() {
    LOG_ERROR("Emergency stop triggered!");
    
    if (m_motorController) {
        m_motorController->emergencyStop();
    }
    
    if (m_mainWindow) {
        QMessageBox::critical(m_mainWindow.get(), 
                            "Emergency Stop", 
                            "Emergency stop has been triggered. All motors stopped.");
    }
}

void Application::onCriticalError(const QString& error) {
    LOG_ERROR(QString("Critical error: %1").arg(error));
    
    if (m_mainWindow) {
        QMessageBox::critical(m_mainWindow.get(), 
                            "Critical Error", 
                            error);
    }
    
    // 可能需要关闭应用程序
    emit shutdownRequested();
}

bool Application::initializeHardware() {
    // 创建硬件接口
    m_serialInterface = std::make_unique<SerialInterface>();
    m_motorInterface = std::make_unique<MotorInterface>(m_serialInterface.get());
    m_sensorInterface = std::make_unique<SensorInterface>(m_serialInterface.get());
    
    return true;
}

bool Application::initializeCore() {
    // 创建核心组件
    m_safetyManager = std::make_unique<SafetyManager>();
    m_motorController = std::make_unique<MotorController>(
        m_motorInterface.get(), m_safetyManager.get());
    m_sensorManager = std::make_unique<SensorManager>(m_sensorInterface.get());
    m_calibrationEngine = std::make_unique<CalibrationEngine>();
    m_dataRecorder = std::make_unique<DataRecorder>();
    
    return true;
}

bool Application::initializeData() {
    // 创建数据处理组件
    m_dataProcessor = std::make_unique<DataProcessor>();
    m_exportManager = std::make_unique<ExportManager>();
    m_fileManager = std::make_unique<FileManager>();
    
    return true;
}

bool Application::initializeUI() {
    // 创建主窗口
    m_mainWindow = std::make_unique<MainWindow>(
        m_serialInterface.get(),
        m_motorController.get(),
        m_sensorManager.get(),
        m_dataRecorder.get()
    );
    
    return true;
}

void Application::connectSignals() {
    // 安全管理器信号
    connect(m_safetyManager.get(), &SafetyManager::emergencyStopTriggered,
            this, &Application::onEmergencyStop);
    
    // 传感器数据处理
    connect(m_sensorManager.get(), &SensorManager::dataUpdated,
            m_dataProcessor.get(), &DataProcessor::processData);
    
    connect(m_dataProcessor.get(), &DataProcessor::dataProcessed,
            m_dataRecorder.get(), &DataRecorder::recordData);
    
    // 错误处理
    connect(m_serialInterface.get(), &SerialInterface::errorOccurred,
            this, &Application::onCriticalError);
    
    // UI关闭信号
    if (m_mainWindow) {
        connect(m_mainWindow.get(), &MainWindow::destroyed,
                this, &Application::shutdown);
    }
}