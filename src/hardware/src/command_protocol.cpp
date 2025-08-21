#include "../include/command_protocol.h"
#include "../../models/include/sensor_data.h"
#include "../../utils/include/string_utils.h"
#include "../../models/include/physics_constants.h"
#include <sstream>
#include <algorithm>
#include <cctype>

std::string CommandProtocol::buildSetHeightCommand(double height) {
    std::ostringstream oss;
    oss << height;
    return formatCommand(CMD_SET_HEIGHT, oss.str());
}

std::string CommandProtocol::buildSetAngleCommand(double angle) {
    std::ostringstream oss;
    oss << angle;
    return formatCommand(CMD_SET_ANGLE, oss.str());
}

std::string CommandProtocol::buildMoveCommand(double height, double angle) {
    std::ostringstream oss;
    oss << height << PARAM_SEPARATOR << angle;
    return formatCommand(CMD_MOVE_TO, oss.str());
}

std::string CommandProtocol::buildStopCommand() {
    return formatCommand(CMD_STOP);
}

std::string CommandProtocol::buildEmergencyStopCommand() {
    return formatCommand(CMD_EMERGENCY_STOP);
}

std::string CommandProtocol::buildHomeCommand() {
    return formatCommand(CMD_HOME);
}

std::string CommandProtocol::buildGetSensorsCommand() {
    return formatCommand(CMD_GET_SENSORS);
}

std::string CommandProtocol::buildGetStatusCommand() {
    return formatCommand(CMD_GET_STATUS);
}

std::string CommandProtocol::buildCustomCommand(const std::string& cmd, const std::string& params) {
    return formatCommand(cmd, params);
}

std::string CommandProtocol::buildBatchCommand(const std::vector<std::string>& commands) {
    std::ostringstream oss;
    oss << CMD_BATCH << SEPARATOR << commands.size() << TERMINATOR;
    
    for (const auto& cmd : commands) {
        oss << cmd << TERMINATOR;
    }
    
    return oss.str();
}

CommandResponse CommandProtocol::parseResponse(const std::string& response) {
    CommandResponse result;
    
    // 检查是否为空或无终止符
    if (response.empty() || !hasTerminator(response)) {
        result.type = ResponseType::UNKNOWN;
        result.success = false;
        return result;
    }
    
    // 去除终止符
    std::string cleanResponse = removeTerminator(response);
    
    // 分割类型和数据
    size_t colonPos = cleanResponse.find(SEPARATOR);
    std::string type = (colonPos != std::string::npos) ? 
                       cleanResponse.substr(0, colonPos) : cleanResponse;
    std::string data = (colonPos != std::string::npos) ? 
                       cleanResponse.substr(colonPos + 1) : "";
    
    // 解析响应类型
    if (type == RSP_OK) {
        result.type = ResponseType::OK;
        result.success = true;
        result.data = data;
    }
    else if (type == RSP_ERROR) {
        result.type = ResponseType::ERROR;
        result.success = false;
        result.errorMessage = data;
    }
    else if (type == RSP_SENSORS) {
        result.type = ResponseType::SENSOR_DATA;
        result.success = true;
        result.data = data;
        
        // 尝试解析传感器数据
        SensorData sensorData;
        if (sensorData.parseFromString(data)) {
            result.sensorData = sensorData;
        }
    }
    else if (type == RSP_STATUS) {
        result.type = ResponseType::STATUS;
        result.success = true;
        result.data = data;
    }
    else {
        result.type = ResponseType::UNKNOWN;
        result.success = false;
    }
    
    return result;
}

bool CommandProtocol::isValidCommand(const std::string& command) {
    if (command.empty() || !hasTerminator(command)) {
        return false;
    }
    
    std::string cleanCmd = removeTerminator(command);
    if (cleanCmd.empty()) {
        return false;
    }
    
    // 检查是否是已知命令
    std::vector<std::string> knownCommands = {
        CMD_SET_HEIGHT, CMD_SET_ANGLE, CMD_MOVE_TO,
        CMD_STOP, CMD_EMERGENCY_STOP, CMD_HOME,
        CMD_GET_SENSORS, CMD_GET_STATUS
    };
    
    size_t colonPos = cleanCmd.find(SEPARATOR);
    std::string cmdType = (colonPos != std::string::npos) ? 
                          cleanCmd.substr(0, colonPos) : cleanCmd;
    
    return std::find(knownCommands.begin(), knownCommands.end(), cmdType) != knownCommands.end();
}

bool CommandProtocol::isValidResponse(const std::string& response) {
    if (response.empty() || !hasTerminator(response)) {
        return false;
    }
    
    std::string cleanRsp = removeTerminator(response);
    if (cleanRsp.empty()) {
        return false;
    }
    
    // 检查是否是已知响应类型
    std::vector<std::string> knownResponses = {
        RSP_OK, RSP_ERROR, RSP_SENSORS, RSP_STATUS
    };
    
    size_t colonPos = cleanRsp.find(SEPARATOR);
    std::string rspType = (colonPos != std::string::npos) ? 
                          cleanRsp.substr(0, colonPos) : cleanRsp;
    
    return std::find(knownResponses.begin(), knownResponses.end(), rspType) != knownResponses.end();
}

bool CommandProtocol::isResponseValidForCommand(const std::string& command, const std::string& response) {
    std::string cleanCmd = removeTerminator(command);
    CommandResponse rsp = parseResponse(response);
    
    // 提取命令类型
    size_t colonPos = cleanCmd.find(SEPARATOR);
    std::string cmdType = (colonPos != std::string::npos) ? 
                          cleanCmd.substr(0, colonPos) : cleanCmd;
    
    // 检查响应是否匹配命令
    if (cmdType == CMD_GET_SENSORS) {
        return rsp.type == ResponseType::SENSOR_DATA;
    }
    else if (cmdType == CMD_GET_STATUS) {
        return rsp.type == ResponseType::STATUS;
    }
    else {
        // 其他命令期望OK或ERROR响应
        return rsp.type == ResponseType::OK || rsp.type == ResponseType::ERROR;
    }
}

ErrorCode CommandProtocol::parseErrorCode(const std::string& errorStr) {
    if (errorStr == "OUT_OF_RANGE") return ErrorCode::OUT_OF_RANGE;
    if (errorStr == "INVALID_COMMAND") return ErrorCode::INVALID_COMMAND;
    if (errorStr == "TIMEOUT") return ErrorCode::TIMEOUT;
    if (errorStr == "HARDWARE_ERROR") return ErrorCode::HARDWARE_ERROR;
    if (errorStr == "BUSY") return ErrorCode::BUSY;
    if (errorStr == "NOT_READY") return ErrorCode::NOT_READY;
    return ErrorCode::UNKNOWN;
}

std::string CommandProtocol::getErrorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::NONE:
            return "No error";
        case ErrorCode::OUT_OF_RANGE:
            return "Value out of range";
        case ErrorCode::INVALID_COMMAND:
            return "Invalid command format";
        case ErrorCode::TIMEOUT:
            return "Communication timeout";
        case ErrorCode::HARDWARE_ERROR:
            return "Hardware error";
        case ErrorCode::BUSY:
            return "Device busy";
        case ErrorCode::NOT_READY:
            return "Device not ready";
        case ErrorCode::UNKNOWN:
        default:
            return "Unknown error";
    }
}

std::string CommandProtocol::formatCommand(const std::string& cmd, const std::string& params) {
    if (params.empty()) {
        return cmd + TERMINATOR;
    }
    return cmd + SEPARATOR + params + TERMINATOR;
}

bool CommandProtocol::hasTerminator(const std::string& str) {
    return str.length() >= 2 && 
           str.substr(str.length() - 2) == TERMINATOR;
}

std::string CommandProtocol::removeTerminator(const std::string& str) {
    if (hasTerminator(str)) {
        return str.substr(0, str.length() - 2);
    }
    return str;
}