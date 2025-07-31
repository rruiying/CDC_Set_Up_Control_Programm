#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileInfo>

class FileManager : public QObject {
    Q_OBJECT
    
public:
    struct FileInfo {
        QString fileName;
        QString filePath;
        qint64 fileSize;
        QDateTime created;
        QDateTime modified;
        QString type;
    };
    
    explicit FileManager(QObject* parent = nullptr);
    ~FileManager();
    
    // 文件操作
    bool createDirectory(const QString& path);
    bool deleteFile(const QString& fileName);
    bool copyFile(const QString& source, const QString& destination);
    bool moveFile(const QString& source, const QString& destination);
    bool renameFile(const QString& oldName, const QString& newName);
    
    // 文件查询
    QStringList listFiles(const QString& directory, 
                         const QStringList& filters = QStringList());
    QList<FileInfo> getFileInfoList(const QString& directory);
    bool fileExists(const QString& fileName);
    qint64 getFileSize(const QString& fileName);
    
    // 数据文件管理
    QString getDataDirectory() const;
    QString getExportDirectory() const;
    QString getConfigDirectory() const;
    QString getLogDirectory() const;
    
    // 清理
    bool cleanOldFiles(const QString& directory, int daysOld);
    qint64 getDirectorySize(const QString& directory);
    
    // 备份
    bool backupFile(const QString& fileName);
    bool restoreFile(const QString& backupName);
    
signals:
    void fileCreated(const QString& fileName);
    void fileDeleted(const QString& fileName);
    void fileModified(const QString& fileName);
    void error(const QString& message);
    
private:
    QString m_baseDirectory;
    
    void ensureDirectoriesExist();
    QString generateBackupName(const QString& originalName) const;
};