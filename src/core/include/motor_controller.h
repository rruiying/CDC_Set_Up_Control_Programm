// src/core/include/motor_controller.h
#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <chrono>
#include <vector>
#include "../../models/include/system_config.h"
#include "../../hardware/include/command_protocol.h"

// 前向声明
class SerialInterface;
class SafetyManager;

/**
 * @brief 电机状态枚举
 */
enum class MotorStatus {
    IDLE,       // 空闲
    MOVING,     // 移动中
    ERROR,      // 错误状态
    HOMING,     // 回零中
    CALIBRATING // 校准中
};

/**
 * @brief 电机命令类型
 */
enum class MotorCommandType {
    SET_HEIGHT,
    SET_ANGLE,
    MOVE_TO,
    STOP,
    HOME
};

/**
 * @brief 电机命令结构
 */
struct MotorCommand {
    MotorCommandType type;
    double height;
    double angle;
};

/**
 * @brief 错误信息结构
 */
struct MotorError {
    int64_t timestamp;
    std::string message;
    ErrorCode code;
};

/**
 * @brief 电机控制器类
 * 
 * 负责电机控制逻辑：
 * - 发送控制命令
 * - 跟踪电机状态
 * - 安全检查
 * - 异步移动控制
 * - 错误处理
 */
class MotorController {
public:
    /**
     * @brief 构造函数
     * @param serialInterface 串口接口
     * @param safetyManager 安全管理器
     */
    MotorController(std::shared_ptr<SerialInterface> serialInterface,
                    std::shared_ptr<SafetyManager> safetyManager);
    
    /**
     * @brief 析构函数
     */
    ~MotorController();
    
    // 回调函数类型
    using StatusCallback = std::function<void(MotorStatus)>;
    using ProgressCallback = std::function<void(double)>; // 0-100%
    using ErrorCallback = std::function<void(const MotorError&)>;
    
    // 基本控制方法
    bool setHeight(double height);
    bool setAngle(double angle);
    bool moveToPosition(double height, double angle);
    bool stop();
    bool emergencyStop();
    bool home();
    
    // 异步控制
    void moveToPositionAsync(double height, double angle);
    bool waitForCompletion(int timeoutMs = 30000);
    
    // 批量命令
    bool executeBatch(const std::vector<MotorCommand>& commands);
    
    // 状态查询
    MotorStatus getStatus() const { return status; }
    bool isMoving() const { return status == MotorStatus::MOVING; }
    bool updateStatus(); // 主动查询MCU状态
    
    // 位置信息
    double getCurrentHeight() const { return currentHeight; }
    double getCurrentAngle() const { return currentAngle; }
    double getTargetHeight() const { return targetHeight; }
    double getTargetAngle() const { return targetAngle; }
    
    // 速度控制
    void setSpeed(MotorSpeed speed);
    MotorSpeed getSpeed() const { return speed; }
    
    // 配置
    void setCommandTimeout(int timeoutMs) { commandTimeout = timeoutMs; }
    int getCommandTimeout() const { return commandTimeout; }
    
    // 回调设置
    void setStatusCallback(StatusCallback callback);
    void setProgressCallback(ProgressCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    // 错误处理
    MotorError getLastError() const;
    void clearError();
    bool hasError() const { return status == MotorStatus::ERROR; }
    
    // 手动位置更新（用于同步）
    void updateCurrentPosition(double height, double angle);
    
private:
    // 内部方法
    bool sendCommand(const std::string& command);
    bool sendCommandAndWait(const std::string& command);
    void monitorMovement();
    void notifyStatus(MotorStatus newStatus);
    void notifyProgress(double progress);
    void notifyError(const std::string& message, ErrorCode code = ErrorCode::UNKNOWN);
    double calculateProgress() const;
    bool checkSafety(double height, double angle);
    int64_t getCurrentTimestamp() const;
    
    // 成员变量
    std::shared_ptr<SerialInterface> serial;
    std::shared_ptr<SafetyManager> safety;
    
    // 状态
    std::atomic<MotorStatus> status{MotorStatus::IDLE};
    std::atomic<double> currentHeight{0.0};
    std::atomic<double> currentAngle{0.0};
    std::atomic<double> targetHeight{0.0};
    std::atomic<double> targetAngle{0.0};
    
    // 配置
    std::atomic<MotorSpeed> speed{MotorSpeed::MEDIUM};
    std::atomic<int> commandTimeout{5000}; // 默认5秒超时
    
    // 线程控制
    std::unique_ptr<std::thread> monitorThread;
    std::atomic<bool> stopMonitoring{false};
    mutable std::mutex mutex;
    
    // 回调
    StatusCallback statusCallback;
    ProgressCallback progressCallback;
    ErrorCallback errorCallback;
    
    // 错误信息
    MotorError lastError;
    
    // 移动起始位置（用于计算进度）
    double moveStartHeight;
    double moveStartAngle;
};

#endif // MOTOR_CONTROLLER_H