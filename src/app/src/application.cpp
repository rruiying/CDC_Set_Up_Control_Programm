// src/app/src/application.cpp
#include "../include/application.h"
#include <QApplication>
#include <QMessageBox>
#include <QMainWindow>  // 添加这个
#include <QWidget> 
#include <QDir>
#include "../../ui/include/mainwindow.h"
#include "../../hardware/include/serial_interface.h"
#include "../../hardware/include/motor_interface.h"
#include "../../hardware/include/sensor_interface.h"
#include "../../core/include/motor_controller.h"
#include "../../core/include/sensor_manager.h"
#include "../../core/include/safety_manager.h"
#include "../../core/include/data_recorder.h"
#include "../../data/include/data_processor.h"
#include "../../data/include/export_manager.h"
#include "../../data/include/file_manager.h"
#include "../../utils/include/logger.h"
#include "../../utils/include/config_manager.h"
#include "../../models/include/system_config.h"

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
    
    // 创建必要的目录
    QDir dir;
    dir.mkpath("./runtime/data");
    dir.mkpath("./runtime/logs");
    dir.mkpath("./runtime/config");
    
    // 加载系统配置
    SystemConfig::getInstance().loadFromFile("./runtime/config/system_config.json");
    
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
        m_sensorManager->stop();
    }
    
    if (m_motorController) {
        m_motorController->stop();
    }
    
    // 停止数据记录
    if (m_dataRecorder) {
        m_dataRecorder->setAutoSave(false);  // 使用正确的方法
    }
    
    // 断开串口
    if (m_serialInterface && m_serialInterface->isOpen()) {
        m_serialInterface->close();
    }
    
    // 保存配置
    SystemConfig::getInstance().saveToFile("./runtime/config/system_config.json");
    
    m_isRunning = false;
    LOG_INFO("Application shutdown complete");
}

void Application::showMainWindow() {
    if (m_mainWindow) {
        // MainWindow 继承自 QMainWindow，应该有 show() 方法
        QMainWindow* qmw = m_mainWindow.get();
        if (qmw) {
            qmw->show();
        }
    }
}

void Application::onEmergencyStop() {
    LOG_ERROR("Emergency stop triggered!");
    
    if (m_motorController) {
        m_motorController->emergencyStop();
    }
    
    if (m_mainWindow) {
        // 使用 m_mainWindow.get() 直接作为 QWidget*
        QMessageBox::critical(m_mainWindow.get(), 
                            "Emergency Stop", 
                            "Emergency stop has been triggered. All motors stopped.");
    }
}

void Application::onCriticalError(const QString& error) {
    LOG_ERROR("Critical error: " + error.toStdString());
    
    if (m_mainWindow) {
        QMessageBox::critical(m_mainWindow.get(), 
                            "Critical Error", 
                            error);
    }
    
    emit shutdownRequested();
}

bool Application::initializeHardware() {
    try {
        // 创建硬件接口
        m_serialInterface = std::make_shared<SerialInterface>();
        
        // 注意：某些类可能还没有实现，暂时注释掉
        // m_motorInterface = std::make_unique<MotorInterface>(m_serialInterface);
        // m_sensorInterface = std::make_unique<SensorInterface>(m_serialInterface);
        
        LOG_INFO("Hardware layer initialized");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Hardware initialization failed: " + std::string(e.what()));
        return false;
    }
}

bool Application::initializeCore() {
    try {
        // 1. 先创建 SafetyManager (shared_ptr)
        m_safetyManager = std::make_shared<SafetyManager>();
        
        // 2. 创建 MotorController - 使用正确的构造函数参数
        m_motorController = std::make_unique<MotorController>(
            m_serialInterface,
            m_safetyManager  // 现在都是 shared_ptr
        );
        
        // 3. 创建其他组件
        m_sensorManager = std::make_unique<SensorManager>(m_serialInterface);
        m_dataRecorder = std::make_unique<DataRecorder>();
        
        LOG_INFO("Core layer initialized");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Core initialization failed: " + std::string(e.what()));
        return false;
    }
}

bool Application::initializeData() {
    try {
        // 创建数据处理组件
        m_dataProcessor = std::make_unique<DataProcessor>();
        m_exportManager = std::make_unique<ExportManager>();
        m_fileManager = std::make_unique<FileManager>();
        
        LOG_INFO("Data layer initialized");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Data initialization failed: " + std::string(e.what()));
        return false;
    }
}

bool Application::initializeUI() {
    try {
        // 创建主窗口
        m_mainWindow = std::make_unique<MainWindow>();
        
        LOG_INFO("UI initialized");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("UI initialization failed: " + std::string(e.what()));
        return false;
    }
}

void Application::connectSignals() {
    // 使用回调机制代替Qt信号
    
    // 安全管理器回调
    if (m_safetyManager) {
        // 使用已有的紧急停止回调
        m_safetyManager->setEmergencyStopCallback([this](bool stopped) {
            if (stopped) {
                // 在主线程中执行
                QMetaObject::invokeMethod(this, "onEmergencyStop", Qt::QueuedConnection);
            }
        });
        
        // 违规回调
        m_safetyManager->setViolationCallback([this](const std::string& reason) {
            LOG_WARNING("Safety violation: " + reason);
        });
    }
    
    // 传感器管理器回调
    if (m_sensorManager) {
        // 数据接收回调
        m_sensorManager->setDataCallback([this](const SensorData& data) {
            if (m_dataRecorder && m_dataRecorder->isRecording()) {
                // 创建测量数据
                MeasurementData measurement(
                    m_motorController ? m_motorController->getCurrentHeight() : 0.0,
                    m_motorController ? m_motorController->getCurrentAngle() : 0.0,
                    data
                );
                m_dataRecorder->recordMeasurement(measurement);
            }
        });
        
        // 错误回调
        m_sensorManager->setErrorCallback([this](const std::string& error) {
            QString qError = QString::fromStdString(error);
            QMetaObject::invokeMethod(this, "onCriticalError", 
                                    Qt::QueuedConnection, 
                                    Q_ARG(QString, qError));
        });
    }
    
    // 数据记录器回调
    if (m_dataRecorder) {
        m_dataRecorder->setDataChangeCallback([](int recordCount) {
            LOG_INFO_F("Data recorder: %d records", recordCount);
        });
    }
    
    // 串口错误处理
    if (m_serialInterface) {
        m_serialInterface->setErrorCallback([this](const std::string& error) {
            QString qError = QString::fromStdString(error);
            QMetaObject::invokeMethod(this, "onCriticalError", 
                                    Qt::QueuedConnection, 
                                    Q_ARG(QString, qError));
        });
    }
    
    // UI关闭信号 - 这个仍然使用Qt信号，因为MainWindow是QObject
    if (m_mainWindow) {
        // 使用 QObject::connect，不是成员函数
        QObject::connect(m_mainWindow.get(), &MainWindow::destroyed,
                        this, &Application::shutdown);
    }
    
    LOG_INFO("Signal connections established");
}