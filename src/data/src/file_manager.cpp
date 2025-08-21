#include "../include/file_manager.h"
#include "../../utils/include/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>

namespace fs = std::filesystem;

FileManager::FileManager(const std::string& baseDirectory)
    : m_baseDirectory(baseDirectory) {
    ensureDirectoriesExist();
    LOG_INFO("FileManager initialized with base directory: " + m_baseDirectory);
}

FileManager::~FileManager() {
    LOG_INFO("FileManager destroyed");
}

// ===== 文件操作 =====

bool FileManager::createDirectory(const std::string& path) {
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        try {
            if (fs::create_directories(path)) {
                LOG_INFO("Created directory: " + path);
                ok = true;
            } else if (fs::exists(path) && fs::is_directory(path)) {
                ok = true;
            } else {
                setError("Failed to create directory: " + path);
            }
        } catch (const fs::filesystem_error& e) {
            setError("Filesystem error: " + std::string(e.what()));
            LOG_ERROR("Failed to create directory: " + path + " - " + e.what());
        }
    }
    if (!ok) notifyError("Failed to create directory: " + path);
    return ok;
}

bool FileManager::deleteFile(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        if (fs::remove(fileName)) {
            LOG_INFO("Deleted file: " + fileName);
            notifyFileDeleted(fileName);
            return true;
        }
        
        setError("File not found: " + fileName);
        return false;
        
    } catch (const fs::filesystem_error& e) {
        setError("Failed to delete file: " + std::string(e.what()));
        LOG_ERROR("Failed to delete file: " + fileName + " - " + e.what());
        notifyError("Failed to delete file: " + fileName);
        return false;
    }
}

bool FileManager::copyFile(const std::string& source, const std::string& destination) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // 如果目标文件存在，先删除
        if (fs::exists(destination)) {
            fs::remove(destination);
        }
        
        fs::copy_file(source, destination);
        LOG_INFO("Copied " + source + " to " + destination);
        notifyFileCreated(destination);
        return true;
        
    } catch (const fs::filesystem_error& e) {
        setError("Failed to copy file: " + std::string(e.what()));
        LOG_ERROR("Failed to copy " + source + " to " + destination + " - " + e.what());
        notifyError("Failed to copy file");
        return false;
    }
}

bool FileManager::moveFile(const std::string& source, const std::string& destination) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        fs::rename(source, destination);
        LOG_INFO("Moved " + source + " to " + destination);
        notifyFileDeleted(source);
        notifyFileCreated(destination);
        return true;
        
    } catch (const fs::filesystem_error& e) {
        // 如果rename失败（可能跨文件系统），尝试复制后删除
        if (copyFile(source, destination)) {
            return deleteFile(source);
        }
        
        setError("Failed to move file: " + std::string(e.what()));
        LOG_ERROR("Failed to move " + source + " to " + destination + " - " + e.what());
        return false;
    }
}

bool FileManager::renameFile(const std::string& oldName, const std::string& newName) {
    return moveFile(oldName, newName);
}

// ===== 文件查询 =====

std::vector<std::string> FileManager::listFiles(const std::string& directory, 
                                               const std::vector<std::string>& filters) {
    std::lock_guard<std::mutex> lock(m_mutex);
    struct Item { std::string name; std::filesystem::file_time_type mtime; };
    std::vector<Item> items;

    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                // 如果没有过滤器，添加所有文件
                if (filters.empty()) {
                    items.push_back({filename, fs::last_write_time(entry)});
                } else {
                    // 检查文件扩展名是否在过滤器中
                    std::string ext = entry.path().extension().string();
                    if (std::find(filters.begin(), filters.end(), ext) != filters.end()) {
                        items.push_back({filename, fs::last_write_time(entry)});
                    }
                }
            }
        }
        
        // 按修改时间排序（最新的在前）
        std::sort(items.begin(), items.end(),
                  [](const Item& a, const Item& b){ return a.mtime > b.mtime; });

    } catch (const fs::filesystem_error& e) {
        setError("Failed to list files: " + std::string(e.what()));
        LOG_ERROR("Failed to list files in " + directory + " - " + e.what());
    }
    
    std::vector<std::string> files;
    files.reserve(items.size());
    for (auto& it : items) files.push_back(it.name);
    return files;
}

std::vector<FileInfo> FileManager::getFileInfoList(const std::string& directory) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FileInfo> fileList;
    
    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                FileInfo info;
                info.fileName = entry.path().filename().string();
                info.filePath = entry.path().string();
                info.fileSize = entry.file_size();
                info.extension = entry.path().extension().string();
                
                // 获取文件时间
                auto ftime = fs::last_write_time(entry);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                info.modified = sctp;
                info.created = sctp; // C++17 filesystem不能可靠地获取创建时间
                
                fileList.push_back(info);
            }
        }
    } catch (const fs::filesystem_error& e) {
        setError("Failed to get file info: " + std::string(e.what()));
        LOG_ERROR("Failed to get file info from " + directory + " - " + e.what());
    }
    
    return fileList;
}

bool FileManager::fileExists(const std::string& fileName) {
    return fs::exists(fileName) && fs::is_regular_file(fileName);
}

uintmax_t FileManager::getFileSize(const std::string& fileName) {
    try {
        return fs::file_size(fileName);
    } catch (const fs::filesystem_error& e) {
        setError("Failed to get file size: " + std::string(e.what()));
        return 0;
    }
}

// ===== 目录管理 =====

std::string FileManager::getDataDirectory() const {
    return m_baseDirectory;
}

std::string FileManager::getExportDirectory() const {
    return m_baseDirectory + "exports/";
}

std::string FileManager::getConfigDirectory() const {
    return m_baseDirectory + "configs/";
}

std::string FileManager::getLogDirectory() const {
    return m_baseDirectory + "logs/";
}

// ===== 清理和维护 =====

bool FileManager::cleanOldFiles(const std::string& directory, int daysOld) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto cutoffTime = std::chrono::system_clock::now() - std::chrono::hours(24 * daysOld);
    int deletedCount = 0;
    
    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto ftime = fs::last_write_time(entry);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                
                if (sctp < cutoffTime) {
                    if (deleteFile(entry.path().string())) {
                        deletedCount++;
                    }
                }
            }
        }
        
        LOG_INFO_F("Cleaned %d old files from %s", deletedCount, directory.c_str());
        return true;
        
    } catch (const fs::filesystem_error& e) {
        setError("Failed to clean old files: " + std::string(e.what()));
        LOG_ERROR("Failed to clean old files from " + directory + " - " + e.what());
        return false;
    }
}

uintmax_t FileManager::getDirectorySize(const std::string& directory) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uintmax_t size = 0;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                size += entry.file_size();
            }
        }
    } catch (const fs::filesystem_error& e) {
        setError("Failed to get directory size: " + std::string(e.what()));
        LOG_ERROR("Failed to get size of " + directory + " - " + e.what());
    }
    
    return size;
}

// ===== 备份和恢复 =====

bool FileManager::backupFile(const std::string& fileName) {
    if (!fileExists(fileName)) {
        setError("File not found: " + fileName);
        notifyError("File not found: " + fileName);
        return false;
    }
    
    std::string backupName = generateBackupName(fileName);
    return copyFile(fileName, backupName);
}

bool FileManager::restoreFile(const std::string& backupName) {
    // 从备份文件名提取原始文件名
    std::string originalName = backupName;
    
    // 移除.backup后缀
    size_t pos = originalName.find(".backup");
    if (pos != std::string::npos) {
        originalName = originalName.substr(0, pos);
    }
    
    // 移除时间戳
    std::regex timestampRegex("_\\d{8}_\\d{6}");
    originalName = std::regex_replace(originalName, timestampRegex, "");
    
    return copyFile(backupName, originalName);
}

// ===== 回调设置 =====

void FileManager::setFileCreatedCallback(FileCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fileCreatedCallback = callback;
}

void FileManager::setFileDeletedCallback(FileCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fileDeletedCallback = callback;
}

void FileManager::setFileModifiedCallback(FileCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fileModifiedCallback = callback;
}

void FileManager::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorCallback = callback;
}

// ===== 实用方法 =====

std::string FileManager::getFileExtension(const std::string& fileName) {
    fs::path p(fileName);
    return p.extension().string();
}

std::string FileManager::getBaseName(const std::string& fileName) {
    fs::path p(fileName);
    return p.stem().string();
}

std::string FileManager::getLastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

// ===== 私有方法 =====

void FileManager::ensureDirectoriesExist() {
    createDirectory(getDataDirectory());
    createDirectory(getExportDirectory());
    createDirectory(getConfigDirectory());
    createDirectory(getLogDirectory());
}

std::string FileManager::generateBackupName(const std::string& originalName) const {
    fs::path p(originalName);
    
    // 获取当前时间戳
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::stringstream ss;
    ss << p.parent_path().string() << "/"
       << p.stem().string() << "_"
       << std::put_time(&tm, "%Y%m%d_%H%M%S")
       << ".backup";
    
    return ss.str();
}

void FileManager::notifyFileCreated(const std::string& fileName) {
   FileCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callback = m_fileCreatedCallback;
    }
    if (callback) {
        callback(fileName);
    }
}

void FileManager::notifyFileDeleted(const std::string& fileName) {
    FileCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callback = m_fileCreatedCallback;
    }
    if (callback) {
        callback(fileName);
    }
}

void FileManager::notifyFileModified(const std::string& fileName) {
    FileCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callback = m_fileModifiedCallback;
    }
    if (callback) {
        callback(fileName);
    }
}

void FileManager::notifyError(const std::string& message) {
    FileCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callback = m_errorCallback;
    }
    if (callback) {
        callback(message);
    }
}

void FileManager::setError(const std::string& error) const {
    m_lastError = error;
}