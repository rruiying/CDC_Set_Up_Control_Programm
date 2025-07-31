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
    
    // 记录到日志
    Logger::instance().log(Logger::Debug, txt);
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
    
    // 设置样式（可选）
    QFile styleFile(":/styles/app_style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QTextStream stream(&styleFile);
        qtApp.setStyleSheet(stream.readAll());
    }
    
    // 创建并初始化应用程序
    Application app;
    
    if (!app.initialize()) {
        QMessageBox::critical(nullptr, 
                            "Initialization Error",
                            "Failed to initialize application. Check logs for details.");
        return -1;
    }
    
    // 显示主窗口
    app.showMainWindow();
    
    // 运行事件循环
    int result = qtApp.exec();
    
    // 清理
    app.shutdown();
    
    return result;
}