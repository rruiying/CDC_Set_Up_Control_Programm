#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include "app/include/application.h"
#include "utils/include/logger.h"

// 自定义消息处理器
void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString txt;
    switch (type) {
        case QtDebugMsg:
            txt = QString("Debug: %1").arg(msg);
            break;
        case QtInfoMsg:
            txt = QString("Info: %1").arg(msg);
            break;
        case QtWarningMsg:
            txt = QString("Warning: %1").arg(msg);
            break;
        case QtCriticalMsg:
            txt = QString("Critical: %1").arg(msg);
            break;
        case QtFatalMsg:
            txt = QString("Fatal: %1").arg(msg);
            abort();
    }
    
    // 记录到日志 - 使用正确的Logger接口
    // 将Qt消息类型映射到LogLevel
    switch (type) {
        case QtDebugMsg:
        case QtInfoMsg:
            Logger::getInstance().info(txt.toStdString());
            break;
        case QtWarningMsg:
            Logger::getInstance().warning(txt.toStdString());
            break;
        case QtCriticalMsg:
        case QtFatalMsg:
            Logger::getInstance().error(txt.toStdString());
            break;
    }
}

int main(int argc, char *argv[]) {
    QApplication qtApp(argc, argv);
    
    // 设置应用程序信息
    qtApp.setOrganizationName("TUM LSE");
    qtApp.setOrganizationDomain("tum.de");
    qtApp.setApplicationName("CDC Control Program");
    qtApp.setApplicationVersion("1.0.0");
    
    // 安装消息处理器
    qInstallMessageHandler(messageHandler);
    
    // 初始化日志系统
    Logger::getInstance().setLogFile("./runtime/logs/cdc_control.log");
    Logger::getInstance().setLogLevel(LogLevel::INFO);
    Logger::getInstance().info("CDC Control Program starting...");
    
    // 设置样式（可选）
    QFile styleFile(":/styles/app_style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QTextStream stream(&styleFile);
        qtApp.setStyleSheet(stream.readAll());
    }
    
    // 创建并初始化应用程序
    Application app;
    
    if (!app.initialize()) {
        Logger::getInstance().error("Failed to initialize application");
        QMessageBox::critical(nullptr, 
                            "Initialization Error",
                            "Failed to initialize application. Check logs for details.");
        return -1;
    }
    
    // 显示主窗口
    app.showMainWindow();
    
    Logger::getInstance().info("Application initialized successfully");
    
    // 运行事件循环
    int result = qtApp.exec();
    
    // 清理
    Logger::getInstance().info("Shutting down application...");
    app.shutdown();
    
    Logger::getInstance().info("CDC Control Program terminated");
    
    return result;
}