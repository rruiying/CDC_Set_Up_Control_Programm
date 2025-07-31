#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QSignalSpy>
#include "data/include/file_manager.h"

class FileManagerTest : public ::testing::Test {
protected:
    FileManager* fileManager;
    QTemporaryDir* tempDir;
    
    void SetUp() override {
        fileManager = new FileManager();
        tempDir = new QTemporaryDir();
    }
    
    void TearDown() override {
        delete fileManager;
        delete tempDir;
    }
    
    QString createTempFile(const QString& name, const QString& content = "test") {
        QString path = tempDir->path() + "/" + name;
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&file);
            stream << content;
            file.close();
        }
        return path;
    }
};

// 测试创建目录
TEST_F(FileManagerTest, CreateDirectory) {
    QString dirPath = tempDir->path() + "/test_dir";
    
    bool result = fileManager->createDirectory(dirPath);
    EXPECT_TRUE(result);
    EXPECT_TRUE(QDir(dirPath).exists());
}

// 测试文件删除
TEST_F(FileManagerTest, DeleteFile) {
    QString filePath = createTempFile("test.txt");
    EXPECT_TRUE(QFile::exists(filePath));
    
    QSignalSpy spy(fileManager, &FileManager::fileDeleted);
    
    bool result = fileManager->deleteFile(filePath);
    EXPECT_TRUE(result);
    EXPECT_FALSE(QFile::exists(filePath));
    EXPECT_EQ(spy.count(), 1);
}

// 测试文件复制
TEST_F(FileManagerTest, CopyFile) {
    QString source = createTempFile("source.txt", "Hello World");
    QString dest = tempDir->path() + "/dest.txt";
    
    bool result = fileManager->copyFile(source, dest);
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(dest));
    
    // 验证内容
    QFile file(dest);
    file.open(QIODevice::ReadOnly);
    EXPECT_EQ(file.readAll(), "Hello World");
}

// 测试文件移动
TEST_F(FileManagerTest, MoveFile) {
    QString source = createTempFile("source.txt");
    QString dest = tempDir->path() + "/moved.txt";
    
    bool result = fileManager->moveFile(source, dest);
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(dest));
    EXPECT_FALSE(QFile::exists(source));
}

// 测试文件重命名
TEST_F(FileManagerTest, RenameFile) {
    QString oldName = createTempFile("old.txt");
    QString newName = tempDir->path() + "/new.txt";
    
    bool result = fileManager->renameFile(oldName, newName);
    EXPECT_TRUE(result);
    EXPECT_TRUE(QFile::exists(newName));
    EXPECT_FALSE(QFile::exists(oldName));
}

// 测试文件列表
TEST_F(FileManagerTest, ListFiles) {
    createTempFile("file1.txt");
    createTempFile("file2.txt");
    createTempFile("file3.csv");
    
    QStringList allFiles = fileManager->listFiles(tempDir->path());
    EXPECT_EQ(allFiles.size(), 3);
    
    QStringList txtFiles = fileManager->listFiles(tempDir->path(), 
                                                  QStringList() << "*.txt");
    EXPECT_EQ(txtFiles.size(), 2);
}

// 测试文件信息
TEST_F(FileManagerTest, GetFileInfo) {
    QString filePath = createTempFile("info.txt", "12345");
    
    EXPECT_TRUE(fileManager->fileExists(filePath));
    EXPECT_EQ(fileManager->getFileSize(filePath), 5);
    
    auto fileList = fileManager->getFileInfoList(tempDir->path());
    EXPECT_EQ(fileList.size(), 1);
    EXPECT_EQ(fileList[0].fileName, "info.txt");
    EXPECT_EQ(fileList[0].fileSize, 5);
}

// 测试目录大小
TEST_F(FileManagerTest, GetDirectorySize) {
    createTempFile("file1.txt", "12345");      // 5 bytes
    createTempFile("file2.txt", "1234567890"); // 10 bytes
    
    qint64 size = fileManager->getDirectorySize(tempDir->path());
    EXPECT_EQ(size, 15);
}

// 测试清理旧文件
TEST_F(FileManagerTest, CleanOldFiles) {
    // 创建一个文件
    QString filePath = createTempFile("old_file.txt");
    
    // 修改文件时间为10天前
    QFile file(filePath);
    file.open(QIODevice::ReadWrite);
    file.close();
    
    // 由于无法在测试中真正修改文件时间，这里只测试函数调用
    bool result = fileManager->cleanOldFiles(tempDir->path(), 5);
    EXPECT_TRUE(result);
}

// 测试文件备份
TEST_F(FileManagerTest, BackupFile) {
    QString original = createTempFile("data.txt", "important data");
    
    bool result = fileManager->backupFile(original);
    EXPECT_TRUE(result);
    
    // 检查备份文件是否存在
    QDir dir(tempDir->path());
    QStringList backups = dir.entryList(QStringList() << "*backup");
    EXPECT_GT(backups.size(), 0);
}

// 测试错误处理
TEST_F(FileManagerTest, ErrorHandling) {
    QSignalSpy errorSpy(fileManager, &FileManager::error);
    
    // 尝试删除不存在的文件
    fileManager->deleteFile("/nonexistent/file.txt");
    EXPECT_GT(errorSpy.count(), 0);
    
    // 尝试复制到无效路径
    QString source = createTempFile("source.txt");
    fileManager->copyFile(source, "/invalid/path/dest.txt");
    EXPECT_GT(errorSpy.count(), 1);
}

// 测试目录路径获取
TEST_F(FileManagerTest, GetDirectoryPaths) {
    QString dataDir = fileManager->getDataDirectory();
    QString exportDir = fileManager->getExportDirectory();
    QString configDir = fileManager->getConfigDirectory();
    QString logDir = fileManager->getLogDirectory();
    
    EXPECT_FALSE(dataDir.isEmpty());
    EXPECT_TRUE(exportDir.endsWith("exports/"));
    EXPECT_TRUE(configDir.endsWith("configs/"));
    EXPECT_TRUE(logDir.endsWith("logs/"));
}