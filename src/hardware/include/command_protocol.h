// src/hardware/include/command_protocol.h
#ifndef COMMAND_PROTOCOL_H
#define COMMAND_PROTOCOL_H

#include <string>
#include <vector>
#include <optional>
#include "../../models/include/sensor_data.h"

/**
 * @brief 响应类型枚举
 */
enum class ResponseType {
    OK,           // 成功响应
    ERROR,        // 错误响应
    SENSOR_DATA,  // 传感器数据
    STATUS,       // 状态信息
    UNKNOWN       // 未知响应
};

/**
 * @brief 错误代码枚举
 */
enum class ErrorCode {
    NONE = 0,
    OUT_OF_RANGE,
    INVALID_COMMAND,
    TIMEOUT,
    HARDWARE_ERROR,
    BUSY,
    NOT_READY,
    UNKNOWN
};

/**
 * @brief 命令响应结构
 */
struct CommandResponse {
    ResponseType type = ResponseType::UNKNOWN;
    bool success = false;
    std::string data;
    std::string errorMessage;
    std::optional<SensorData> sensorData;
    
    CommandResponse() = default;
};

/**
 * @brief 通信协议类
 * 
 * 定义与MCU通信的协议格式：
 * - 命令格式：COMMAND[:PARAMS]\r\n
 * - 响应格式：TYPE[:DATA]\r\n
 */
class CommandProtocol {
public:
    // 命令构建方法
    static std::string buildSetHeightCommand(double height);
    static std::string buildSetAngleCommand(double angle);
    static std::string buildMoveCommand(double height, double angle);
    static std::string buildStopCommand();
    static std::string buildEmergencyStopCommand();
    static std::string buildHomeCommand();
    static std::string buildGetSensorsCommand();
    static std::string buildGetStatusCommand();
    static std::string buildCustomCommand(const std::string& cmd, const std::string& params);
    
    // 批量命令
    static std::string buildBatchCommand(const std::vector<std::string>& commands);
    
    // 响应解析方法
    static CommandResponse parseResponse(const std::string& response);
    
    // 验证方法
    static bool isValidCommand(const std::string& command);
    static bool isValidResponse(const std::string& response);
    static bool isResponseValidForCommand(const std::string& command, const std::string& response);
    
    // 错误处理
    static ErrorCode parseErrorCode(const std::string& errorStr);
    static std::string getErrorMessage(ErrorCode code);
    
    // 协议常量
    static constexpr const char* TERMINATOR = "\r\n";
    static constexpr const char* SEPARATOR = ":";
    static constexpr const char* PARAM_SEPARATOR = ",";
    
    // 命令常量
    static constexpr const char* CMD_SET_HEIGHT = "SET_HEIGHT";
    static constexpr const char* CMD_SET_ANGLE = "SET_ANGLE";
    static constexpr const char* CMD_MOVE_TO = "MOVE_TO";
    static constexpr const char* CMD_STOP = "STOP";
    static constexpr const char* CMD_EMERGENCY_STOP = "EMERGENCY_STOP";
    static constexpr const char* CMD_HOME = "HOME";
    static constexpr const char* CMD_GET_SENSORS = "GET_SENSORS";
    static constexpr const char* CMD_GET_STATUS = "GET_STATUS";
    static constexpr const char* CMD_BATCH = "BATCH";
    
    // 响应常量
    static constexpr const char* RSP_OK = "OK";
    static constexpr const char* RSP_ERROR = "ERROR";
    static constexpr const char* RSP_SENSORS = "SENSORS";
    static constexpr const char* RSP_STATUS = "STATUS";
    
private:
    // 辅助方法
    static std::string formatCommand(const std::string& cmd, const std::string& params = "");
    static std::vector<std::string> splitString(const std::string& str, const std::string& delimiter);
    static std::string trimString(const std::string& str);
    static bool hasTerminator(const std::string& str);
    static std::string removeTerminator(const std::string& str);
};

#endif // COMMAND_PROTOCOL_H