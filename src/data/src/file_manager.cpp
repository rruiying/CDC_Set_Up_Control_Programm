#include "../include/file_manager.h"
#include "../../utils/include/logger.h"
#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QRegularExpression>

FileManager::FileManager(QObject* parent)
    : QObject(parent)
    , m_baseDirectory("./runtime/data/")
{
    ensureDirectoriesExist();
}

FileManager::~FileManager() {
}

bool FileManager::createDirectory(const QString& path) {
    QDir dir;
    if (dir.mkpath(path)) {
        LOG_INFO("Created directory: " + path.toStdString());
        return true;
    } else {
        LOG_ERROR("Failed to create directory: " + path.toStdString());
        emit error(QString("Failed to create directory: %1").arg(path));
        return false;
    }
}

bool FileManager::deleteFile(const QString& fileName) {
    QFile file(fileName);
    if (file.remove()) {
        LOG_INFO("Deleted file: " + fileName.toStdString());
        emit fileDeleted(fileName);
        return true;
    } else {
        LOG_ERROR("Failed to delete file: " + fileName.toStdString());
        emit error(QString("Failed to delete file: %1").arg(fileName));
        return false;
    }
}

bool FileManager::copyFile(const QString& source, const QString& destination) {
    // 如果目标文件存在，先删除
    if (QFile::exists(destination)) {
        QFile::remove(destination);
    }
    
    if (QFile::copy(source, destination)) {
        LOG_INFO("Copied " + source.toStdString() + " to " + destination.toStdString());
        emit fileCreated(destination);
        return true;
    } else {
        LOG_ERROR("Failed to copy " + source.toStdString() + " to " + destination.toStdString());
        emit error(QString("Failed to copy file"));
        return false;
    }
}

bool FileManager::moveFile(const QString& source, const QString& destination) {
    if (copyFile(source, destination)) {
        return deleteFile(source);
    }
    return false;
}

bool FileManager::renameFile(const QString& oldName, const QString& newName) {
    QFile file(oldName);
    if (file.rename(newName)) {
        LOG_INFO("Renamed " + oldName.toStdString() + " to " + newName.toStdString());
        emit fileModified(newName);
        return true;
    } else {
        LOG_ERROR("Failed to rename " + oldName.toStdString() + " to " + newName.toStdString());
        emit error(QString("Failed to rename file"));
        return false;
    }
}

QStringList FileManager::listFiles(const QString& directory, 
                                  const QStringList& filters) {
    QDir dir(directory);
    return dir.entryList(filters, QDir::Files, QDir::Time);
}

QList<FileManager::FileInfo> FileManager::getFileInfoList(const QString& directory) {
    QList<FileInfo> fileList;
    QDir dir(directory);
    
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
        FileInfo fileInfo;
        fileInfo.fileName = info.fileName();
        fileInfo.filePath = info.filePath();
        fileInfo.fileSize = info.size();
        fileInfo.created = info.birthTime();
        fileInfo.modified = info.lastModified();
        fileInfo.type = info.suffix();
        
        fileList.append(fileInfo);
    }
    
    return fileList;
}

bool FileManager::fileExists(const QString& fileName) {
    return QFile::exists(fileName);
}

qint64 FileManager::getFileSize(const QString& fileName) {
    QFileInfo info(fileName);
    return info.size();
}

QString FileManager::getDataDirectory() const {
    return m_baseDirectory;
}

QString FileManager::getExportDirectory() const {
    return m_baseDirectory + "exports/";
}

QString FileManager::getConfigDirectory() const {
    return m_baseDirectory + "configs/";
}

QString FileManager::getLogDirectory() const {
    return m_baseDirectory + "logs/";
}

bool FileManager::cleanOldFiles(const QString& directory, int daysOld) {
    QDir dir(directory);
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-daysOld);
    int deletedCount = 0;
    
    QDirIterator it(directory, QDir::Files);
    while (it.hasNext()) {
        it.next();
        QFileInfo info(it.filePath());
        
        if (info.lastModified() < cutoffDate) {
            if (deleteFile(it.filePath())) {
                deletedCount++;
            }
        }
    }
    
    LOG_INFO_F("Cleaned %d old files from %s", deletedCount, directory.toStdString().c_str());
    return true;
}

qint64 FileManager::getDirectorySize(const QString& directory) {
    qint64 size = 0;
    
    QDirIterator it(directory, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        size += it.fileInfo().size();
    }
    
    return size;
}

bool FileManager::backupFile(const QString& fileName) {
    if (!fileExists(fileName)) {
        emit error(QString("File not found: %1").arg(fileName));
        return false;
    }
    
    QString backupName = generateBackupName(fileName);
    return copyFile(fileName, backupName);
}

bool FileManager::restoreFile(const QString& backupName) {
    // 从备份文件名提取原始文件名
    QString originalName = backupName;
    originalName.replace(".backup", "");
    originalName.replace(QRegularExpression("_\\d{8}_\\d{6}"), "");
    
    return copyFile(backupName, originalName);
}

void FileManager::ensureDirectoriesExist() {
    createDirectory(getDataDirectory());
    createDirectory(getExportDirectory());
    createDirectory(getConfigDirectory());
    createDirectory(getLogDirectory());
}

QString FileManager::generateBackupName(const QString& originalName) const {
    QFileInfo info(originalName);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    
    return QString("%1/%2_%3.backup")
           .arg(info.path())
           .arg(info.baseName())
           .arg(timestamp);
}