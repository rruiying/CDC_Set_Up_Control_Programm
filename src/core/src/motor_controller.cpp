// src/core/src/motor_controller.cpp
#include "../include/motor_controller.h"
#include "../include/safety_manager.h"
#include "../../hardware/include/serial_interface.h"
#include "../../hardware/include/command_protocol.h"
#include "../../utils/include/logger.h"
#include <sstream>
#include <cmath>

MotorController::MotorController(std::shared_ptr<SerialInterface> serialInterface,
                                 std::shared_ptr<SafetyManager> safetyManager)
    : serial(serialInterface), safety(safetyManager) {
    
    // 从系统配置加载速度设置
    speed = SystemConfig::getInstance().getMotorSpeed();
    
    LOG_INFO("MotorController initialized");
}

MotorController::~MotorController() {
    // 停止监控线程
    stopMonitoring = true;
    if (monitorThread && monitorThread->joinable()) {
        monitorThread->join();
    }
}

bool MotorController::setHeight(double height) {
    // 安全检查
    if (!checkSafety(height, currentAngle)) {
        notifyError("Height out of safety limits", ErrorCode::OUT_OF_RANGE);
        return false;
    }
    
    std::string command = CommandProtocol::buildSetHeightCommand(height);
    if (sendCommandAndWait(command)) {
        targetHeight = height;
        LOG_INFO_F("Height set to %.1f mm", height);
        return true;
    }
    
    return false;
}

bool MotorController::setAngle(double angle) {
    // 安全检查
    if (!checkSafety(currentHeight, angle)) {
        notifyError("Angle out of safety limits", ErrorCode::OUT_OF_RANGE);
        return false;
    }
    
    std::string command = CommandProtocol::buildSetAngleCommand(angle);
    if (sendCommandAndWait(command)) {
        targetAngle = angle;
        LOG_INFO_F("Angle set to %.1f degrees", angle);
        return true;
    }
    
    return false;
}

bool MotorController::moveToPosition(double height, double angle) {
    // 安全检查
    if (!checkSafety(height, angle)) {
        notifyError("Position out of safety limits", ErrorCode::OUT_OF_RANGE);
        return false;
    }
    
    // 记录起始位置
    moveStartHeight = currentHeight.load();
    moveStartAngle = currentAngle.load();
    targetHeight = height;
    targetAngle = angle;
    
    std::string command = CommandProtocol::buildMoveCommand(height, angle);
    if (!sendCommand(command)) {
        return false;
    }
    
    // 启动监控线程
    stopMonitoring = false;
    monitorThread = std::make_unique<std::thread>(&MotorController::monitorMovement, this);
    
    // 等待移动完成
    return waitForCompletion();
}

bool MotorController::stop() {
    std::string command = CommandProtocol::buildStopCommand();
    bool success = sendCommandAndWait(command);
    
    if (success) {
        notifyStatus(MotorStatus::IDLE);
        LOG_INFO("Motor stopped");
    }
    
    return success;
}

bool MotorController::emergencyStop() {
    std::string command = CommandProtocol::buildEmergencyStopCommand();
    bool success = sendCommand(command); // 不等待响应，立即返回
    
    notifyStatus(MotorStatus::ERROR);
    stopMonitoring = true;
    
    LOG_WARNING("Emergency stop activated");
    return success;
}

bool MotorController::home() {
    std::string command = CommandProtocol::buildHomeCommand();
    if (!sendCommand(command)) {
        return false;
    }
    
    // 设置目标位置为原点
    auto config = SystemConfig::getInstance();
    targetHeight = config.getHomeHeight();
    targetAngle = config.getHomeAngle();
    
    notifyStatus(MotorStatus::HOMING);
    
    // 启动监控
    stopMonitoring = false;
    monitorThread = std::make_unique<std::thread>(&MotorController::monitorMovement, this);
    
    LOG_INFO("Homing started");
    return true;
}

void MotorController::moveToPositionAsync(double height, double angle) {
    // 安全检查
    if (!checkSafety(height, angle)) {
        notifyError("Position out of safety limits", ErrorCode::OUT_OF_RANGE);
        return;
    }
    
    // 记录起始位置
    moveStartHeight = currentHeight.load();
    moveStartAngle = currentAngle.load();
    targetHeight = height;
    targetAngle = angle;
    
    // 在新线程中执行移动
    std::thread([this, height, angle]() {
        std::string command = CommandProtocol::buildMoveCommand(height, angle);
        if (sendCommand(command)) {
            // 启动监控
            monitorMovement();
        }
    }).detach();
}

bool MotorController::waitForCompletion(int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();
    
    while (isMoving()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
            
        if (elapsed >= timeoutMs) {
            LOG_ERROR("Motor movement timeout");
            stop();
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return !hasError();
}

bool MotorController::executeBatch(const std::vector<MotorCommand>& commands) {
    for (const auto& cmd : commands) {
        bool success = false;
        
        switch (cmd.type) {
            case MotorCommandType::SET_HEIGHT:
                success = setHeight(cmd.height);
                break;
                
            case MotorCommandType::SET_ANGLE:
                success = setAngle(cmd.angle);
                break;
                
            case MotorCommandType::MOVE_TO:
                success = moveToPosition(cmd.height, cmd.angle);
                break;
                
            case MotorCommandType::STOP:
                success = stop();
                break;
                
            case MotorCommandType::HOME:
                success = home();
                break;
        }
        
        if (!success) {
            LOG_ERROR("Batch command failed, stopping execution");
            return false;
        }
    }
    
    return true;
}

bool MotorController::updateStatus() {
    std::string command = CommandProtocol::buildGetStatusCommand();
    std::string response = serial->sendAndReceive(command, commandTimeout);
    
    if (response.empty()) {
        notifyError("Status query timeout", ErrorCode::TIMEOUT);
        return false;
    }
    
    CommandResponse cmdResponse = CommandProtocol::parseResponse(response);
    
    if (cmdResponse.type == ResponseType::STATUS) {
        // 解析状态数据 "STATUS:READY,25.0,5.5"
        std::istringstream iss(cmdResponse.data);
        std::string statusStr;
        double height, angle;
        char comma;
        
        std::getline(iss, statusStr, ',');
        iss >> height >> comma >> angle;
        
        // 更新位置
        updateCurrentPosition(height, angle);
        
        // 更新状态
        if (statusStr == "READY") {
            notifyStatus(MotorStatus::IDLE);
        } else if (statusStr == "MOVING") {
            notifyStatus(MotorStatus::MOVING);
        } else if (statusStr == "ERROR") {
            notifyStatus(MotorStatus::ERROR);
        }
        
        return true;
    }
    
    return false;
}

void MotorController::setSpeed(MotorSpeed newSpeed) {
    speed = newSpeed;
    SystemConfig::getInstance().setMotorSpeed(newSpeed);
    LOG_INFO_F("Motor speed set to %s", SystemConfig::getInstance().getMotorSpeedString().c_str());
}

void MotorController::setStatusCallback(StatusCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    statusCallback = callback;
}

void MotorController::setProgressCallback(ProgressCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    progressCallback = callback;
}

void MotorController::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    errorCallback = callback;
}

MotorError MotorController::getLastError() const {
    std::lock_guard<std::mutex> lock(mutex);
    return lastError;
}

void MotorController::clearError() {
    std::lock_guard<std::mutex> lock(mutex);
    lastError = MotorError{0, "", ErrorCode::NONE};
    if (status == MotorStatus::ERROR) {
        notifyStatus(MotorStatus::IDLE);
    }
}

void MotorController::updateCurrentPosition(double height, double angle) {
    currentHeight = height;
    currentAngle = angle;
    
    // 计算并通知进度
    if (isMoving()) {
        double progress = calculateProgress();
        notifyProgress(progress);
    }
}

// 私有方法实现

bool MotorController::sendCommand(const std::string& command) {
    if (!serial || !serial->isOpen()) {
        notifyError("Serial port not open", ErrorCode::HARDWARE_ERROR);
        return false;
    }
    
    return serial->sendCommand(command);
}

bool MotorController::sendCommandAndWait(const std::string& command) {
    if (!serial || !serial->isOpen()) {
        notifyError("Serial port not open", ErrorCode::HARDWARE_ERROR);
        return false;
    }
    
    std::string response = serial->sendAndReceive(command, commandTimeout);
    
    if (response.empty()) {
        notifyError("Command timeout", ErrorCode::TIMEOUT);
        return false;
    }
    
    CommandResponse cmdResponse = CommandProtocol::parseResponse(response);
    
    if (cmdResponse.type == ResponseType::OK) {
        return true;
    } else if (cmdResponse.type == ResponseType::ERROR) {
        ErrorCode code = CommandProtocol::parseErrorCode(cmdResponse.errorMessage);
        notifyError(cmdResponse.errorMessage, code);
        return false;
    }
    
    notifyError("Unexpected response", ErrorCode::UNKNOWN);
    return false;
}

void MotorController::monitorMovement() {
    notifyStatus(MotorStatus::MOVING);
    
    while (!stopMonitoring && isMoving()) {
        // 查询当前状态
        if (!updateStatus()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // 检查是否到达目标
        double heightDiff = std::abs(currentHeight - targetHeight);
        double angleDiff = std::abs(currentAngle - targetAngle);
        
        if (heightDiff < 0.1 && angleDiff < 0.1) {
            // 到达目标位置
            notifyStatus(MotorStatus::IDLE);
            notifyProgress(100.0);
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    stopMonitoring = false;
}

void MotorController::notifyStatus(MotorStatus newStatus) {
    if (status != newStatus) {
        status = newStatus;
        LOG_INFO_F("Motor status changed to: %d", static_cast<int>(newStatus));
        
        if (statusCallback) {
            std::thread([this, newStatus]() {
                statusCallback(newStatus);
            }).detach();
        }
    }
}

void MotorController::notifyProgress(double progress) {
    if (progressCallback) {
        std::thread([this, progress]() {
            progressCallback(progress);
        }).detach();
    }
}

void MotorController::notifyError(const std::string& message, ErrorCode code) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        lastError = MotorError{getCurrentTimestamp(), message, code};
    }
    
    notifyStatus(MotorStatus::ERROR);
    LOG_ERROR("Motor error: " + message);
    
    if (errorCallback) {
        std::thread([this, error = lastError]() {
            errorCallback(error);
        }).detach();
    }
}

double MotorController::calculateProgress() const {
    double heightRange = std::abs(targetHeight - moveStartHeight);
    double angleRange = std::abs(targetAngle - moveStartAngle);
    
    if (heightRange < 0.01 && angleRange < 0.01) {
        return 100.0; // 已经在目标位置
    }
    
    double heightProgress = 0.0;
    double angleProgress = 0.0;
    
    if (heightRange > 0.01) {
        double heightMoved = std::abs(currentHeight - moveStartHeight);
        heightProgress = (heightMoved / heightRange) * 100.0;
    } else {
        heightProgress = 100.0;
    }
    
    if (angleRange > 0.01) {
        double angleMoved = std::abs(currentAngle - moveStartAngle);
        angleProgress = (angleMoved / angleRange) * 100.0;
    } else {
        angleProgress = 100.0;
    }
    
    // 返回两者的平均进度
    double progress = (heightProgress + angleProgress) / 2.0;
    return std::min(100.0, std::max(0.0, progress));
}

bool MotorController::checkSafety(double height, double angle) {
    // 使用SafetyManager进行检查
    if (safety && !safety->checkPosition(height, angle)) {
        return false;
    }
    
    // 也使用系统配置进行检查
    return SystemConfig::getInstance().isPositionValid(height, angle);
}

int64_t MotorController::getCurrentTimestamp() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<milliseconds>(duration).count();
}