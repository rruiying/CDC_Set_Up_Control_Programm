#include "../include/system_config.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// 简单的JSON解析和生成（实际项目中建议使用专门的JSON库）
#include <regex>
#include <iomanip>

// 静态成员初始化
std::unique_ptr<SystemConfig> SystemConfig::instance = nullptr;
std::mutex SystemConfig::instanceMutex;

SystemConfig& SystemConfig::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance.reset(new SystemConfig());
    }
    return *instance;
}

SystemConfig::SystemConfig() {
    // 构造函数使用默认值
}

bool SystemConfig::loadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // 读取整个文件
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    try {
        // 简单的JSON解析（实际项目建议使用nlohmann/json等库）
        // 这里使用正则表达式进行基本解析
        
        // 解析安全限位
        std::regex minHeightRegex(R"("minHeight"\s*:\s*([0-9.+-]+))");
        std::regex maxHeightRegex(R"("maxHeight"\s*:\s*([0-9.+-]+))");
        std::regex minAngleRegex(R"("minAngle"\s*:\s*([0-9.+-]+))");
        std::regex maxAngleRegex(R"("maxAngle"\s*:\s*([0-9.+-]+))");
        
        std::smatch match;
        if (std::regex_search(content, match, minHeightRegex)) {
            minHeight = std::stod(match[1]);
        }
        if (std::regex_search(content, match, maxHeightRegex)) {
            maxHeight = std::stod(match[1]);
        }
        if (std::regex_search(content, match, minAngleRegex)) {
            minAngle = std::stod(match[1]);
        }
        if (std::regex_search(content, match, maxAngleRegex)) {
            maxAngle = std::stod(match[1]);
        }
        
        // 解析电容板参数
        std::regex areaRegex(R"("area"\s*:\s*([0-9.+-]+))");
        std::regex epsilonRegex(R"("dielectricConstant"\s*:\s*([0-9.+-]+))");
        
        if (std::regex_search(content, match, areaRegex)) {
            plateArea = std::stod(match[1]);
        }
        if (std::regex_search(content, match, epsilonRegex)) {
            dielectricConstant = std::stod(match[1]);
        }
        
        // 解析系统尺寸
        std::regex totalHeightRegex(R"("totalHeight"\s*:\s*([0-9.+-]+))");
        std::regex middlePlateRegex(R"("middlePlateHeight"\s*:\s*([0-9.+-]+))");
        std::regex spacingRegex(R"("sensorSpacing"\s*:\s*([0-9.+-]+))");
        
        if (std::regex_search(content, match, totalHeightRegex)) {
            totalHeight = std::stod(match[1]);
        }
        if (std::regex_search(content, match, middlePlateRegex)) {
            middlePlateHeight = std::stod(match[1]);
        }
        if (std::regex_search(content, match, spacingRegex)) {
            sensorSpacing = std::stod(match[1]);
        }

        // 解析原点位置
        std::regex homeHeightRegex(R"("homePosition"[^}]*"height"\s*:\s*([0-9.+-]+))");
        std::regex homeAngleRegex(R"("homePosition"[^}]*"angle"\s*:\s*([0-9.+-]+))");
        
        if (std::regex_search(content, match, homeHeightRegex)) {
            homeHeight = std::stod(match[1]);
        }
        if (std::regex_search(content, match, homeAngleRegex)) {
            homeAngle = std::stod(match[1]);
        }
        
        // 解析通信参数
        std::regex baudRateRegex(R"("defaultBaudRate"\s*:\s*([0-9]+))");
        std::regex timeoutRegex(R"("timeout"\s*:\s*([0-9]+))");
        std::regex retryRegex(R"("retryCount"\s*:\s*([0-9]+))");
        
        if (std::regex_search(content, match, baudRateRegex)) {
            defaultBaudRate = std::stoi(match[1]);
        }
        if (std::regex_search(content, match, timeoutRegex)) {
            communicationTimeout = std::stoi(match[1]);
        }
        if (std::regex_search(content, match, retryRegex)) {
            retryCount = std::stoi(match[1]);
        }
        
        // 解析数据记录参数
        std::regex updateIntervalRegex(R"("sensorUpdateInterval"\s*:\s*([0-9]+))");
        std::regex maxRecordsRegex(R"("maxRecords"\s*:\s*([0-9]+))");
        std::regex autoSaveRegex(R"("autoSaveInterval"\s*:\s*([0-9]+))");
        
        if (std::regex_search(content, match, updateIntervalRegex)) {
            sensorUpdateInterval = std::stoi(match[1]);
        }
        if (std::regex_search(content, match, maxRecordsRegex)) {
            maxRecords = std::stoi(match[1]);
        }
        if (std::regex_search(content, match, autoSaveRegex)) {
            autoSaveInterval = std::stoi(match[1]);
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool SystemConfig::saveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << std::fixed << std::setprecision(1);
    file << "{\n";
    
    // 安全限位
    file << "    \"safetyLimits\": {\n";
    file << "        \"minHeight\": " << minHeight << ",\n";
    file << "        \"maxHeight\": " << maxHeight << ",\n";
    file << "        \"minAngle\": " << minAngle << ",\n";
    file << "        \"maxAngle\": " << maxAngle << "\n";
    file << "    },\n";
    
    // 电容板参数
    file << "    \"capacitorPlate\": {\n";
    file << "        \"area\": " << std::setprecision(3) << plateArea << ",\n";
    file << "        \"dielectricConstant\": " << dielectricConstant << "\n";
    file << "    },\n";
    
    // 系统尺寸
    file << "    \"systemDimensions\": {\n";
    file << "        \"totalHeight\": " << std::setprecision(1) << totalHeight << ",\n";
    file << "        \"middlePlateHeight\": " << middlePlateHeight << ",\n";
    file << "        \"sensorSpacing\": " << sensorSpacing << "\n";
    file << "    },\n";
    
    // 电机控制
    file << "    \"motorControl\": {\n";
    file << "        \"homePosition\": {\n";
    file << "            \"height\": " << homeHeight << ",\n";
    file << "            \"angle\": " << homeAngle << "\n";
    file << "        }\n";
    file << "    },\n";
    
    // 通信参数
    file << "    \"communication\": {\n";
    file << "        \"defaultBaudRate\": " << defaultBaudRate << ",\n";
    file << "        \"timeout\": " << communicationTimeout << ",\n";
    file << "        \"retryCount\": " << retryCount << "\n";
    file << "    },\n";
    
    // 数据记录参数
    file << "    \"dataRecording\": {\n";
    file << "        \"sensorUpdateInterval\": " << sensorUpdateInterval << ",\n";
    file << "        \"maxRecords\": " << maxRecords << ",\n";
    file << "        \"autoSaveInterval\": " << autoSaveInterval << "\n";
    file << "    }\n";
    
    file << "}\n";
    
    return file.good();
}

void SystemConfig::setSafetyLimits(double minH, double maxH, double minA, double maxA) {
    if (!validateSafetyLimits(minH, maxH, minA, maxA)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    minHeight = minH;
    maxHeight = maxH;
    minAngle = minA;
    maxAngle = maxA;
    notifyChange();
}

void SystemConfig::setHeightLimits(double minH, double maxH) {
    if (minH >= maxH) {
        return; // 无效范围
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    this->minHeight = minH;
    this->maxHeight = maxH;
    notifyChange();
}

void SystemConfig::setAngleLimits(double minA, double maxA) {
    if (minA >= maxA) {
        return; // 无效范围
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    this->minAngle = minA;
    this->maxAngle = maxA;
    notifyChange();
}

bool SystemConfig::setPlateArea(double area) {
    if (area <= 0 || area > 1000000) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    plateArea = area;
    notifyChange();
    return true;
}

bool SystemConfig::setDielectricConstant(double epsilon) {
    if (epsilon <= 0) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    dielectricConstant = epsilon;
    notifyChange();
    return true;
}

void SystemConfig::setSystemDimensions(double total, double middle, double spacing) {
    std::lock_guard<std::mutex> lock(mutex);
    totalHeight = total;
    middlePlateHeight = middle;
    sensorSpacing = spacing;
    notifyChange();
}


void SystemConfig::setHomePosition(double height, double angle) {
    std::lock_guard<std::mutex> lock(mutex);
    homeHeight = height;
    homeAngle = angle;
    notifyChange();
}

std::vector<int> SystemConfig::getSupportedBaudRates() const {
    return {9600, 19200, 38400, 57600, 115200};
}

bool SystemConfig::isHeightInRange(double height) const {
    std::lock_guard<std::mutex> lock(mutex);
    return height >= minHeight && height <= maxHeight;
}

bool SystemConfig::isAngleInRange(double angle) const {
    std::lock_guard<std::mutex> lock(mutex);
    return angle >= minAngle && angle <= maxAngle;
}

bool SystemConfig::isPositionValid(double height, double angle) const {
    return isHeightInRange(height) && isAngleInRange(angle);
}

void SystemConfig::setConfigChangeCallback(ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    changeCallback = callback;
}

// 更新reset()方法以使用新的默认值
void SystemConfig::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 重置到新的默认值（符合第二页需求）
    minHeight = 0.0;
    maxHeight = 150.0;        // mm
    minAngle = -90.0;
    maxAngle = 90.0;
    
    plateArea = 2500.0;       // mm² (50mm x 50mm)
    dielectricConstant = 1.0;
    
    totalHeight = 150.0;      // mm
    middlePlateHeight = 25.0; // mm
    sensorSpacing = 80.0;     // mm
    
    homeHeight = 0.0;         // mm，改为0
    homeAngle = 0.0;          // degrees
    
    defaultBaudRate = 115200;
    communicationTimeout = 5000;
    retryCount = 3;
    
    sensorUpdateInterval = 2000;
    maxRecords = 10000;
    autoSaveInterval = 300000;
    
    notifyChange();
}

std::string SystemConfig::getConfigSummary() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::ostringstream oss;
    oss << "System Configuration:\n";
    oss << "  Safety Limits: Height[" << minHeight << "-" << maxHeight << "]mm, ";
    oss << "Angle[" << minAngle << "-" << maxAngle << "]°\n";
    oss << "  Capacitor: Area=" << plateArea << "m², ε_r=" << dielectricConstant << "\n";
    oss << "  System: Height=" << totalHeight << "mm, MiddlePlate=" << middlePlateHeight << "mm\n";
    oss << "  Motor: Home=[" << homeHeight << "mm, " << homeAngle << "°]\n";
    oss << "  Communication: BaudRate=" << defaultBaudRate << ", Timeout=" << communicationTimeout << "ms\n";
    oss << "  Data Recording: UpdateInterval=" << sensorUpdateInterval << "ms, MaxRecords=" << maxRecords;
    
    return oss.str();
}

void SystemConfig::notifyChange() {
    ConfigChangeCallback cb;
    {
        std::lock_guard<std::mutex> lock(mutex);
        cb = changeCallback;
    }
    if (cb) cb();
}

bool SystemConfig::validateSafetyLimits(double minH, double maxH, double minA, double maxA) const {
    return minH < maxH && minA < maxA;
}