#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <filesystem>
#include <mutex>

/**
 * @brief 文件信息结构
 */
struct FileInfo {
    std::string fileName;
    std::string filePath;
    uintmax_t fileSize;
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point modified;
    std::string extension;
};

/**
 * @brief 文件管理器类（纯C++实现）
 * 
 * 提供文件和目录操作功能：
 * - 文件的创建、删除、复制、移动
 * - 目录管理
 * - 文件信息查询
 * - 备份和恢复
 */
class FileManager {
public:
    // 回调函数类型
    using FileCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    /**
     * @brief 构造函数
     * @param baseDirectory 基础目录路径
     */
    explicit FileManager(const std::string& baseDirectory = "./runtime/data/");
    
    /**
     * @brief 析构函数
     */
    ~FileManager();
    
    // ===== 文件操作 =====
    
    /**
     * @brief 创建目录
     * @param path 目录路径
     * @return true 如果成功创建或目录已存在
     */
    bool createDirectory(const std::string& path);
    
    /**
     * @brief 删除文件
     * @param fileName 文件名
     * @return true 如果成功删除
     */
    bool deleteFile(const std::string& fileName);
    
    /**
     * @brief 复制文件
     * @param source 源文件路径
     * @param destination 目标文件路径
     * @return true 如果成功复制
     */
    bool copyFile(const std::string& source, const std::string& destination);
    
    /**
     * @brief 移动文件
     * @param source 源文件路径
     * @param destination 目标文件路径
     * @return true 如果成功移动
     */
    bool moveFile(const std::string& source, const std::string& destination);
    
    /**
     * @brief 重命名文件
     * @param oldName 旧文件名
     * @param newName 新文件名
     * @return true 如果成功重命名
     */
    bool renameFile(const std::string& oldName, const std::string& newName);
    
    // ===== 文件查询 =====
    
    /**
     * @brief 列出目录中的文件
     * @param directory 目录路径
     * @param filters 文件扩展名过滤器（如 {".txt", ".csv"}）
     * @return 文件名列表
     */
    std::vector<std::string> listFiles(const std::string& directory, 
                                      const std::vector<std::string>& filters = {});
    
    /**
     * @brief 获取文件信息列表
     * @param directory 目录路径
     * @return 文件信息列表
     */
    std::vector<FileInfo> getFileInfoList(const std::string& directory);
    
    /**
     * @brief 检查文件是否存在
     * @param fileName 文件名
     * @return true 如果文件存在
     */
    bool fileExists(const std::string& fileName);
    
    /**
     * @brief 获取文件大小
     * @param fileName 文件名
     * @return 文件大小（字节）
     */
    uintmax_t getFileSize(const std::string& fileName);
    
    // ===== 目录管理 =====
    
    /**
     * @brief 获取数据目录路径
     */
    std::string getDataDirectory() const;
    
    /**
     * @brief 获取导出目录路径
     */
    std::string getExportDirectory() const;
    
    /**
     * @brief 获取配置目录路径
     */
    std::string getConfigDirectory() const;
    
    /**
     * @brief 获取日志目录路径
     */
    std::string getLogDirectory() const;
    
    // ===== 清理和维护 =====
    
    /**
     * @brief 清理旧文件
     * @param directory 目录路径
     * @param daysOld 天数阈值
     * @return true 如果成功清理
     */
    bool cleanOldFiles(const std::string& directory, int daysOld);
    
    /**
     * @brief 获取目录大小
     * @param directory 目录路径
     * @return 目录总大小（字节）
     */
    uintmax_t getDirectorySize(const std::string& directory);
    
    // ===== 备份和恢复 =====
    
    /**
     * @brief 备份文件
     * @param fileName 要备份的文件名
     * @return true 如果成功备份
     */
    bool backupFile(const std::string& fileName);
    
    /**
     * @brief 恢复文件
     * @param backupName 备份文件名
     * @return true 如果成功恢复
     */
    bool restoreFile(const std::string& backupName);
    
    // ===== 回调设置 =====
    
    /**
     * @brief 设置文件创建回调
     */
    void setFileCreatedCallback(FileCallback callback);
    
    /**
     * @brief 设置文件删除回调
     */
    void setFileDeletedCallback(FileCallback callback);
    
    /**
     * @brief 设置文件修改回调
     */
    void setFileModifiedCallback(FileCallback callback);
    
    /**
     * @brief 设置错误回调
     */
    void setErrorCallback(ErrorCallback callback);
    
    // ===== 实用方法 =====
    
    /**
     * @brief 获取文件扩展名
     * @param fileName 文件名
     * @return 扩展名（包含点号）
     */
    static std::string getFileExtension(const std::string& fileName);
    
    /**
     * @brief 获取文件基础名（不含扩展名）
     * @param fileName 文件名
     * @return 基础文件名
     */
    static std::string getBaseName(const std::string& fileName);
    
    /**
     * @brief 获取最后的错误信息
     */
    std::string getLastError() const;
    
private:
    // 成员变量
    std::string m_baseDirectory;
    mutable std::string m_lastError;
    mutable std::mutex m_mutex;
    
    // 回调函数
    FileCallback m_fileCreatedCallback;
    FileCallback m_fileDeletedCallback;
    FileCallback m_fileModifiedCallback;
    ErrorCallback m_errorCallback;
    
    // 私有方法
    void ensureDirectoriesExist();
    std::string generateBackupName(const std::string& originalName) const;
    void notifyFileCreated(const std::string& fileName);
    void notifyFileDeleted(const std::string& fileName);
    void notifyFileModified(const std::string& fileName);
    void notifyError(const std::string& message);
    void setError(const std::string& error) const;
};

#endif // FILE_MANAGER_H