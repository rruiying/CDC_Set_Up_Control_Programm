// tests/hardware_tests/test_command_protocol.cpp
#include <gtest/gtest.h>
#include "hardware/include/command_protocol.h"

class CommandProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前的初始化
    }
};

// 测试命令构建
TEST_F(CommandProtocolTest, BuildCommands) {
    // 设置高度命令
    std::string cmd = CommandProtocol::buildSetHeightCommand(25.5);
    EXPECT_EQ(cmd, "SET_HEIGHT:25.5\r\n");
    
    // 设置角度命令
    cmd = CommandProtocol::buildSetAngleCommand(-15.3);
    EXPECT_EQ(cmd, "SET_ANGLE:-15.3\r\n");
    
    // 移动到位置命令
    cmd = CommandProtocol::buildMoveCommand(30.0, 5.5);
    EXPECT_EQ(cmd, "MOVE_TO:30.0,5.5\r\n");
    
    // 停止命令
    cmd = CommandProtocol::buildStopCommand();
    EXPECT_EQ(cmd, "STOP\r\n");
    
    // 紧急停止命令
    cmd = CommandProtocol::buildEmergencyStopCommand();
    EXPECT_EQ(cmd, "EMERGENCY_STOP\r\n");
    
    // 回零命令
    cmd = CommandProtocol::buildHomeCommand();
    EXPECT_EQ(cmd, "HOME\r\n");
    
    // 获取传感器数据命令
    cmd = CommandProtocol::buildGetSensorsCommand();
    EXPECT_EQ(cmd, "GET_SENSORS\r\n");
    
    // 获取状态命令
    cmd = CommandProtocol::buildGetStatusCommand();
    EXPECT_EQ(cmd, "GET_STATUS\r\n");
}

// 测试响应解析 - 成功响应
TEST_F(CommandProtocolTest, ParseSuccessResponses) {
    CommandResponse response;
    
    // OK响应
    response = CommandProtocol::parseResponse("OK\r\n");
    EXPECT_EQ(response.type, ResponseType::OK);
    EXPECT_TRUE(response.success);
    EXPECT_TRUE(response.data.empty());
    
    // OK响应带数据
    response = CommandProtocol::parseResponse("OK:HEIGHT_SET\r\n");
    EXPECT_EQ(response.type, ResponseType::OK);
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.data, "HEIGHT_SET");
}

// 测试响应解析 - 错误响应
TEST_F(CommandProtocolTest, ParseErrorResponses) {
    CommandResponse response;
    
    // 错误响应
    response = CommandProtocol::parseResponse("ERROR:INVALID_COMMAND\r\n");
    EXPECT_EQ(response.type, ResponseType::ERROR);
    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.errorMessage, "INVALID_COMMAND");
    
    // 超出范围错误
    response = CommandProtocol::parseResponse("ERROR:OUT_OF_RANGE\r\n");
    EXPECT_EQ(response.type, ResponseType::ERROR);
    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.errorMessage, "OUT_OF_RANGE");
}

// 测试响应解析 - 传感器数据
TEST_F(CommandProtocolTest, ParseSensorData) {
    CommandResponse response;
    
    // 传感器数据响应
    std::string sensorData = "SENSORS:12.5,13.0,156.2,156.8,23.5,2.5,157.3\r\n";
    response = CommandProtocol::parseResponse(sensorData);
    
    EXPECT_EQ(response.type, ResponseType::SENSOR_DATA);
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.data, "12.5,13.0,156.2,156.8,23.5,2.5,157.3");
    
    // 验证解析后的传感器数据
    EXPECT_TRUE(response.sensorData.has_value());
    if (response.sensorData.has_value()) {
        const SensorData& data = response.sensorData.value();
        EXPECT_DOUBLE_EQ(data.getUpperSensor1(), 12.5);
        EXPECT_DOUBLE_EQ(data.getUpperSensor2(), 13.0);
        EXPECT_DOUBLE_EQ(data.getLowerSensor1(), 156.2);
        EXPECT_DOUBLE_EQ(data.getLowerSensor2(), 156.8);
        EXPECT_DOUBLE_EQ(data.getTemperature(), 23.5);
        EXPECT_DOUBLE_EQ(data.getMeasuredAngle(), 2.5);
        EXPECT_DOUBLE_EQ(data.getMeasuredCapacitance(), 157.3);
    }
}

// 测试响应解析 - 状态数据
TEST_F(CommandProtocolTest, ParseStatusData) {
    CommandResponse response;
    
    // 状态响应
    response = CommandProtocol::parseResponse("STATUS:READY,25.0,5.5\r\n");
    
    EXPECT_EQ(response.type, ResponseType::STATUS);
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.data, "READY,25.0,5.5");
    
    // 移动中状态
    response = CommandProtocol::parseResponse("STATUS:MOVING,30.0,7.5\r\n");
    EXPECT_EQ(response.type, ResponseType::STATUS);
    EXPECT_EQ(response.data, "MOVING,30.0,7.5");
    
    // 错误状态
    response = CommandProtocol::parseResponse("STATUS:ERROR,0.0,0.0\r\n");
    EXPECT_EQ(response.type, ResponseType::STATUS);
    EXPECT_EQ(response.data, "ERROR,0.0,0.0");
}

// 测试无效响应处理
TEST_F(CommandProtocolTest, ParseInvalidResponses) {
    CommandResponse response;
    
    // 空响应
    response = CommandProtocol::parseResponse("");
    EXPECT_EQ(response.type, ResponseType::UNKNOWN);
    EXPECT_FALSE(response.success);
    
    // 无终止符的响应
    response = CommandProtocol::parseResponse("OK");
    EXPECT_EQ(response.type, ResponseType::UNKNOWN);
    
    // 未知命令响应
    response = CommandProtocol::parseResponse("UNKNOWN:DATA\r\n");
    EXPECT_EQ(response.type, ResponseType::UNKNOWN);
    
    // 格式错误的传感器数据
    response = CommandProtocol::parseResponse("SENSORS:12.5,13.0\r\n");
    EXPECT_EQ(response.type, ResponseType::SENSOR_DATA);
    EXPECT_FALSE(response.sensorData.has_value());  // 数据不完整
}

// 测试命令验证
TEST_F(CommandProtocolTest, ValidateCommands) {
    // 有效命令
    EXPECT_TRUE(CommandProtocol::isValidCommand("SET_HEIGHT:25.0\r\n"));
    EXPECT_TRUE(CommandProtocol::isValidCommand("STOP\r\n"));
    EXPECT_TRUE(CommandProtocol::isValidCommand("GET_SENSORS\r\n"));
    
    // 无效命令（无终止符）
    EXPECT_FALSE(CommandProtocol::isValidCommand("SET_HEIGHT:25.0"));
    
    // 无效命令（空）
    EXPECT_FALSE(CommandProtocol::isValidCommand(""));
    
    // 无效命令（只有终止符）
    EXPECT_FALSE(CommandProtocol::isValidCommand("\r\n"));
}

// 测试响应验证
TEST_F(CommandProtocolTest, ValidateResponses) {
    // 有效响应
    EXPECT_TRUE(CommandProtocol::isValidResponse("OK\r\n"));
    EXPECT_TRUE(CommandProtocol::isValidResponse("ERROR:MESSAGE\r\n"));
    EXPECT_TRUE(CommandProtocol::isValidResponse("SENSORS:1,2,3,4,5,6,7\r\n"));
    
    // 无效响应
    EXPECT_FALSE(CommandProtocol::isValidResponse("OK"));
    EXPECT_FALSE(CommandProtocol::isValidResponse(""));
}

// 测试命令和响应的匹配
TEST_F(CommandProtocolTest, CommandResponseMatching) {
    // 发送SET_HEIGHT，期望OK响应
    EXPECT_TRUE(CommandProtocol::isResponseValidForCommand(
        "SET_HEIGHT:25.0\r\n", "OK:HEIGHT_SET\r\n"));
    
    // 发送GET_SENSORS，期望SENSORS响应
    EXPECT_TRUE(CommandProtocol::isResponseValidForCommand(
        "GET_SENSORS\r\n", "SENSORS:1,2,3,4,5,6,7\r\n"));
    
    // 发送GET_STATUS，期望STATUS响应
    EXPECT_TRUE(CommandProtocol::isResponseValidForCommand(
        "GET_STATUS\r\n", "STATUS:READY,25.0,5.5\r\n"));
    
    // 不匹配的响应
    EXPECT_FALSE(CommandProtocol::isResponseValidForCommand(
        "SET_HEIGHT:25.0\r\n", "SENSORS:1,2,3,4,5,6,7\r\n"));
}

// 测试错误代码解析
TEST_F(CommandProtocolTest, ParseErrorCodes) {
    ErrorCode code;
    
    code = CommandProtocol::parseErrorCode("OUT_OF_RANGE");
    EXPECT_EQ(code, ErrorCode::OUT_OF_RANGE);
    
    code = CommandProtocol::parseErrorCode("INVALID_COMMAND");
    EXPECT_EQ(code, ErrorCode::INVALID_COMMAND);
    
    code = CommandProtocol::parseErrorCode("TIMEOUT");
    EXPECT_EQ(code, ErrorCode::TIMEOUT);
    
    code = CommandProtocol::parseErrorCode("HARDWARE_ERROR");
    EXPECT_EQ(code, ErrorCode::HARDWARE_ERROR);
    
    code = CommandProtocol::parseErrorCode("UNKNOWN_ERROR");
    EXPECT_EQ(code, ErrorCode::UNKNOWN);
}

// 测试错误消息生成
TEST_F(CommandProtocolTest, GetErrorMessage) {
    std::string msg;
    
    msg = CommandProtocol::getErrorMessage(ErrorCode::OUT_OF_RANGE);
    EXPECT_EQ(msg, "Value out of range");
    
    msg = CommandProtocol::getErrorMessage(ErrorCode::INVALID_COMMAND);
    EXPECT_EQ(msg, "Invalid command format");
    
    msg = CommandProtocol::getErrorMessage(ErrorCode::TIMEOUT);
    EXPECT_EQ(msg, "Communication timeout");
    
    msg = CommandProtocol::getErrorMessage(ErrorCode::HARDWARE_ERROR);
    EXPECT_EQ(msg, "Hardware error");
}

// 测试自定义命令
TEST_F(CommandProtocolTest, CustomCommands) {
    // 构建自定义命令
    std::string cmd = CommandProtocol::buildCustomCommand("SET_PARAM", "VALUE123");
    EXPECT_EQ(cmd, "SET_PARAM:VALUE123\r\n");
    
    // 无参数的自定义命令
    cmd = CommandProtocol::buildCustomCommand("RESET", "");
    EXPECT_EQ(cmd, "RESET\r\n");
}

// 测试批量命令
TEST_F(CommandProtocolTest, BatchCommands) {
    std::vector<std::string> commands = {
        "SET_HEIGHT:25.0",
        "SET_ANGLE:5.5",
        "MOVE_TO:25.0,5.5"
    };
    
    std::string batch = CommandProtocol::buildBatchCommand(commands);
    EXPECT_EQ(batch, "BATCH:3\r\nSET_HEIGHT:25.0\r\nSET_ANGLE:5.5\r\nMOVE_TO:25.0,5.5\r\n");
}