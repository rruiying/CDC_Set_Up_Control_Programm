#include "../include/device_info.h"
#include "../../utils/include/time_utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

DeviceInfo::DeviceInfo()
    : baudRate(115200),
      deviceType(DeviceType::UNKNOWN),
      connectionStatus(ConnectionStatus::DISCONNECTED),
      lastConnectTime(0),
      lastDisconnectTime(0),
      lastActivityTime(TimeUtils::getCurrentTimestamp()),
      currentSessionStart(0),
      connectionCount(0),
      disconnectionCount(0),
      totalConnectedTime(0),
      errorCount(0),
      lastErrorTime(0) {
    generateDeviceId();
}

DeviceInfo::DeviceInfo(const std::string& name, const std::string& portName, int baudRate)
    : name(name),
      portName(portName),
      baudRate(baudRate),
      connectionStatus(ConnectionStatus::DISCONNECTED),
      lastConnectTime(0),
      lastDisconnectTime(0),
      lastActivityTime(TimeUtils::getCurrentTimestamp()),
      currentSessionStart(0),
      connectionCount(0),
      disconnectionCount(0),
      totalConnectedTime(0),
      errorCount(0),
      lastErrorTime(0) {
    generateDeviceId();
    deviceType = inferDeviceType();
}

DeviceInfo::DeviceInfo(const DeviceInfo& other) {
    *this = other;
}

DeviceInfo& DeviceInfo::operator=(const DeviceInfo& other) {
    if (this != &other) {
        name = other.name;
        portName = other.portName;
        baudRate = other.baudRate;
        deviceId = other.deviceId;
        deviceType = other.deviceType;
        connectionStatus = other.connectionStatus;
        lastConnectTime = other.lastConnectTime;
        lastDisconnectTime = other.lastDisconnectTime;
        lastActivityTime = other.lastActivityTime;
        currentSessionStart = other.currentSessionStart;
        connectionCount = other.connectionCount;
        disconnectionCount = other.disconnectionCount;
        totalConnectedTime = other.totalConnectedTime;
        errorCount = other.errorCount;
        lastErrorTime = other.lastErrorTime;
        lastErrorMessage = other.lastErrorMessage;
    }
    return *this;
}

std::string DeviceInfo::getDeviceTypeString() const {
    switch (deviceType) {
        case DeviceType::MOTOR_CONTROLLER:
            return "Motor Controller";
        case DeviceType::SENSOR:
            return "Sensor";
        case DeviceType::COMBINED:
            return "Combined Device";
        case DeviceType::UNKNOWN:
        default:
            return "Unknown";
    }
}

void DeviceInfo::setConnectionStatus(ConnectionStatus status) {
    if (connectionStatus == status) {
        return;  // 状态未改变
    }

    auto now = TimeUtils::getCurrentTimestamp();

    // 处理状态转换
    if (status == ConnectionStatus::CONNECTED && 
        connectionStatus != ConnectionStatus::CONNECTED) {
        // 连接成功
        lastConnectTime = now;
        currentSessionStart = now;
        connectionCount++;
    } else if (connectionStatus == ConnectionStatus::CONNECTED && 
               status != ConnectionStatus::CONNECTED) {
        // 断开连接
        lastDisconnectTime = now;
        disconnectionCount++;
        
        // 更新总连接时间
        if (currentSessionStart > 0) {
            totalConnectedTime += (now - currentSessionStart);
        }
        currentSessionStart = 0;
    }
    
    connectionStatus = status;
    updateLastActivityTime();
}

std::string DeviceInfo::getConnectionStatusString() const {
    switch (connectionStatus) {
        case ConnectionStatus::DISCONNECTED:
            return "Disconnected";
        case ConnectionStatus::CONNECTING:
            return "Connecting";
        case ConnectionStatus::CONNECTED:
            return "Connected";
        case ConnectionStatus::ERROR:
            return "Error";
        default:
            return "Unknown";
    }
}

void DeviceInfo::updateLastActivityTime() {
    lastActivityTime = TimeUtils::getCurrentTimestamp();
}

int64_t DeviceInfo::getTotalConnectedTime() const {
    int64_t total = totalConnectedTime;
    
    // 如果当前正在连接，加上当前会话时间
    if (connectionStatus == ConnectionStatus::CONNECTED && currentSessionStart > 0) {
        total += (TimeUtils::getCurrentTimestamp() - currentSessionStart);
    }
    
    return total;
}

void DeviceInfo::recordError(const std::string& errorMsg) {
    errorCount++;
    lastErrorTime = TimeUtils::getCurrentTimestamp();
    lastErrorMessage = errorMsg;
    updateLastActivityTime();
}

void DeviceInfo::clearErrorStatistics() {
    errorCount = 0;
    lastErrorTime = 0;
    lastErrorMessage.clear();
}

std::string DeviceInfo::serialize() const {
    std::ostringstream oss;
    oss << "DeviceInfo{";
    oss << "name=\"" << name << "\",";
    oss << "port=\"" << portName << "\",";
    oss << "baud=" << baudRate << ",";
    oss << "type=" << static_cast<int>(deviceType) << ",";
    oss << "id=\"" << deviceId << "\",";
    oss << "status=" << static_cast<int>(connectionStatus) << ",";
    oss << "connections=" << connectionCount << ",";
    oss << "errors=" << errorCount;
    oss << "}";
    return oss.str();
}

bool DeviceInfo::deserialize(const std::string& data) {
    // 简单的解析实现（实际项目中建议使用JSON库）
    if (data.find("DeviceInfo{") != 0 || data.back() != '}') {
        return false;
    }
    
    try {
        // 提取name
        size_t pos = data.find("name=\"");
        if (pos != std::string::npos) {
            pos += 6;
            size_t endPos = data.find("\"", pos);
            name = data.substr(pos, endPos - pos);
        }
        
        // 提取port
        pos = data.find("port=\"");
        if (pos != std::string::npos) {
            pos += 6;
            size_t endPos = data.find("\"", pos);
            portName = data.substr(pos, endPos - pos);
        }
        
        // 提取baud
        pos = data.find("baud=");
        if (pos != std::string::npos) {
            pos += 5;
            size_t endPos = data.find(",", pos);
            baudRate = std::stoi(data.substr(pos, endPos - pos));
        }
        
        // 提取type
        pos = data.find("type=");
        if (pos != std::string::npos) {
            pos += 5;
            size_t endPos = data.find(",", pos);
            deviceType = static_cast<DeviceType>(std::stoi(data.substr(pos, endPos - pos)));
        }
        
        // 提取id
        pos = data.find("id=\"");
        if (pos != std::string::npos) {
            pos += 4;
            size_t endPos = data.find("\"", pos);
            deviceId = data.substr(pos, endPos - pos);
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool DeviceInfo::isSamePort(const DeviceInfo& other) const {
    return portName == other.portName;
}

std::string DeviceInfo::getSummary() const {
    std::ostringstream oss;
    oss << "Device: " << name << " [" << deviceId.substr(0, 8) << "...]" << std::endl;
    oss << "  Port: " << portName << " @ " << baudRate << " baud" << std::endl;
    oss << "  Type: " << getDeviceTypeString() << std::endl;
    oss << "  Status: " << getConnectionStatusString() << std::endl;
    oss << "  Connections: " << connectionCount << " (Total time: " 
        << (getTotalConnectedTime() / 1000) << "s)" << std::endl;
    oss << "  Errors: " << errorCount;
    
    if (errorCount > 0 && !lastErrorMessage.empty()) {
        oss << " (Last: " << lastErrorMessage << ")";
    }
    
    return oss.str();
}

void DeviceInfo::generateDeviceId() {
    // 生成UUID格式的设备ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex_chars = "0123456789ABCDEF";
    std::string uuid = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    
    for (size_t i = 0; i < uuid.length(); ++i) {
        if (uuid[i] == 'x') {
            uuid[i] = hex_chars[dis(gen)];
        } else if (uuid[i] == 'y') {
            uuid[i] = hex_chars[(dis(gen) & 0x3) | 0x8];
        }
    }
    
    deviceId = uuid;
}

DeviceType DeviceInfo::inferDeviceType() const {
    // 根据名称推断设备类型
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    
    if (lowerName.find("motor") != std::string::npos ||
        lowerName.find("controller") != std::string::npos) {
        return DeviceType::MOTOR_CONTROLLER;
    } else if (lowerName.find("sensor") != std::string::npos ||
               lowerName.find("capacitance") != std::string::npos) {
        return DeviceType::SENSOR;
    } else if (lowerName.find("cdc") != std::string::npos ||
               lowerName.find("system") != std::string::npos) {
        return DeviceType::COMBINED;
    }
    
    return DeviceType::UNKNOWN;
}
