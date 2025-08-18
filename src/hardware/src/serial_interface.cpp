// src/hardware/src/serial_interface.cpp
#include "../include/serial_interface.h"
#include "../../utils/include/logger.h"
#include <chrono>
#include <algorithm>
#include <cstring>

// 平台相关头文件
#ifdef _WIN32
    #include <windows.h>
    #include <setupapi.h>
    #include <devguid.h>
    #pragma comment(lib, "setupapi.lib")
#else
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/ioctl.h>
#endif

// 定义 INVALID_HANDLE_VALUE（如果不在 Windows 上）
#ifndef INVALID_HANDLE_VALUE
    #define INVALID_HANDLE_VALUE -1
#endif

// MockSerialPort 类定义（用于测试）
class MockSerialPort : public SerialInterface {
    public:
        std::queue<std::string> mockResponses;
        std::vector<std::string> sentCommands;
    };

// 内部实现类（平台相关）
class SerialInterface::Impl {
public:
#ifdef _WIN32
    Impl() : handle(INVALID_HANDLE_VALUE) {}
#else
    Impl() : fd(-1) {}
#endif
    ~Impl() { close(); }
    
    bool open(const std::string& portName, const SerialPortConfig& config);
    void close();
    bool write(const std::vector<uint8_t>& data);
    std::vector<uint8_t> read(size_t maxBytes, int timeoutMs);
    int bytesAvailable() const;
    void flush();

    // 测试模式支持
    std::queue<std::string> mockResponses;
    std::vector<std::string> mockSentCommands;
    
private:
#ifdef _WIN32
    HANDLE handle;
#else
    int fd;
    struct termios oldTermios;
#endif

    friend class SerialInterface;
};

// 静态方法：获取可用端口
std::vector<SerialPortInfo> SerialInterface::getAvailablePorts() {
    std::vector<SerialPortInfo> ports;
    
#ifdef _WIN32
    // Windows端口枚举
    for (int i = 1; i <= 256; i++) {
        std::string portName = "COM" + std::to_string(i);
        HANDLE hPort = CreateFileA(portName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, NULL);
            
        if (hPort != INVALID_HANDLE_VALUE) {
            CloseHandle(hPort);
            SerialPortInfo info;
            info.portName = portName;
            info.description = "Serial Port " + portName;
            info.isAvailable = true;
            ports.push_back(info);
        }
    }
#else
    // Linux/Unix端口枚举
    DIR* dir = opendir("/dev");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name.find("ttyUSB") != std::string::npos ||
                name.find("ttyACM") != std::string::npos ||
                name.find("ttyS") != std::string::npos) {
                SerialPortInfo info;
                info.portName = "/dev/" + name;
                info.description = "Serial Port " + name;
                info.isAvailable = true;
                ports.push_back(info);
            }
        }
        closedir(dir);
    }
#endif
    
    // 在模拟模式下添加模拟端口
    if (ports.empty()) {
        SerialPortInfo mockPort;
        mockPort.portName = "COM_MOCK";
        mockPort.description = "Mock Serial Port";
        mockPort.isAvailable = true;
        ports.push_back(mockPort);
    }
    
    return ports;
}

SerialInterface::SerialInterface() : pImpl(std::make_unique<Impl>()) {
}

SerialInterface::~SerialInterface() {
    close();
    if (reconnectThreadPtr && reconnectThreadPtr->joinable()) {
        stopReconnect = true;
        reconnectThreadPtr->join();
    }
}

bool SerialInterface::open(const std::string& portName, int baudRate) {
    SerialPortConfig config;
    config.baudRate = baudRate;
    return open(portName, config);
}

bool SerialInterface::open(const std::string& portName, const SerialPortConfig& config) {
    
    if (connected) {
        close();  // close() 会自己获取锁
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    currentPort = portName;
    currentConfig = config;
    
    bool success = false;
    
    if (mockMode) {
        // 模拟模式
        success = true;
        LOG_INFO_F("Mock serial port opened: %s @ %d baud", portName.c_str(), config.baudRate);
    } else {
        // 真实模式
        success = platformOpen(portName, config);
    }
    
    if (success) {
        connected = true;
        notifyConnection(true);
        LOG_INFO_F("Serial port opened: %s @ %d baud", portName.c_str(), config.baudRate);
        
        // 启动自动重连线程
        if (autoReconnect && !reconnectThreadPtr) {
            stopReconnect = false;
            reconnectThreadPtr = std::make_unique<std::thread>(&SerialInterface::reconnectThread, this);
        }
    } else {
        currentPort.clear();
        LOG_ERROR_F("Failed to open serial port: %s", portName.c_str());
    }
    
    return success;
}

void SerialInterface::close() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (connected) {
        connected = false;
        
        if (!mockMode) {
            platformClose();
        }
        
        notifyConnection(false);
        LOG_INFO_F("Serial port closed: %s", currentPort.c_str());
        currentPort.clear();
    }
}

bool SerialInterface::isOpen() const {
    return connected;
}

bool SerialInterface::sendCommand(const std::string& command) {
    if (!connected) {
        LOG_ERROR("Cannot send command: port not open");
        return false;
    }
    
    std::vector<uint8_t> data(command.begin(), command.end());
    
    if (mockMode) {
        // 模拟模式：记录发送的命令
        pImpl->mockSentCommands.push_back(command);
        LOG_INFO_F("Mock TX: %s", command.c_str());
        return true;
    }
    
    return sendData(data);
}

bool SerialInterface::sendData(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!connected) {
        return false;
    }
    
    if (mockMode) {
        return true;
    }
    
    return platformWrite(data);
}

std::string SerialInterface::readLine(int timeoutMs) {
    return readUntilTerminator("\r\n", timeoutMs);
}

std::vector<uint8_t> SerialInterface::readBytes(size_t count, int timeoutMs) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!connected) {
        return {};
    }
    
    if (mockMode) {
        // 模拟模式：返回模拟数据
        if (!pImpl->mockResponses.empty()) {
            std::string response = pImpl->mockResponses.front();
            pImpl->mockResponses.pop();
            return std::vector<uint8_t>(response.begin(), response.end());
        }
        
        // 模拟超时
        std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
        return {};
    }
    
    return platformRead(count, timeoutMs);
}

std::string SerialInterface::sendAndReceive(const std::string& command, int timeoutMs) {
    if (!sendCommand(command)) {
        return "";
    }
    
    return readLine(timeoutMs);
}

void SerialInterface::flushBuffers() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (connected && !mockMode) {
        platformFlush();
    }
}

int SerialInterface::bytesAvailable() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!connected || mockMode) {
        return 0;
    }
    
    return platformBytesAvailable();
}

void SerialInterface::setConnectionCallback(ConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    connectionCallback = callback;
}

void SerialInterface::setDataReceivedCallback(DataReceivedCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    dataReceivedCallback = callback;
}

void SerialInterface::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    errorCallback = callback;
}

// 内部方法实现
void SerialInterface::notifyConnection(bool connected) {
    if (connectionCallback) {
        connectionCallback(connected);
    }
}

void SerialInterface::notifyDataReceived(const std::string& data) {
    if (dataReceivedCallback) {
        dataReceivedCallback(data);
    }
}

void SerialInterface::notifyError(const std::string& error) {
    if (errorCallback) {
        errorCallback(error);
    }
}

std::string SerialInterface::readUntilTerminator(const std::string& terminator, int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();
    std::string buffer;
    
    while (true) {
        // 检查超时
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) {
            break;
        }
        
        // 读取一个字节
        auto bytes = readBytes(1, std::min(10, timeoutMs - static_cast<int>(elapsed)));
        if (!bytes.empty()) {
            buffer += static_cast<char>(bytes[0]);
            
            // 检查是否收到终止符
            if (buffer.size() >= terminator.size() &&
                buffer.substr(buffer.size() - terminator.size()) == terminator) {
                
                // 通知数据接收
                notifyDataReceived(buffer);
                return buffer;
            }
        }
    }
    
    return "";
}

void SerialInterface::reconnectThread() {
    while (!stopReconnect) {
        if (!connected && !currentPort.empty()) {
            LOG_INFO("Attempting to reconnect serial port...");
            if (open(currentPort, currentConfig)) {
                LOG_INFO("Serial port reconnected successfully");
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void SerialInterface::simulateDisconnection() {
    if (mockMode) {
        connected = false;
        notifyConnection(false);
    }
}

// 平台相关的默认实现（可被子类覆盖）
bool SerialInterface::platformOpen(const std::string& portName, const SerialPortConfig& config) {
    return pImpl->open(portName, config);
}

void SerialInterface::platformClose() {
    pImpl->close();
}

bool SerialInterface::platformWrite(const std::vector<uint8_t>& data) {
    return pImpl->write(data);
}

std::vector<uint8_t> SerialInterface::platformRead(size_t maxBytes, int timeoutMs) {
    return pImpl->read(maxBytes, timeoutMs);
}

int SerialInterface::platformBytesAvailable() const {
    return pImpl->bytesAvailable();
}

void SerialInterface::platformFlush() {
    pImpl->flush();
}

// Impl类的平台相关实现
bool SerialInterface::Impl::open(const std::string& portName, const SerialPortConfig& config) {
#ifdef _WIN32
    // Windows实现
    std::string fullPortName = "\\\\.\\" + portName;
    handle = CreateFileA(fullPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
        
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // 配置串口
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    
    if (!GetCommState(handle, &dcb)) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
        return false;
    }
    
    dcb.BaudRate = config.baudRate;
    dcb.ByteSize = static_cast<BYTE>(config.dataBits);
    dcb.StopBits = (config.stopBits == StopBits::ONE) ? ONESTOPBIT : TWOSTOPBITS;
    dcb.Parity = (config.parity == Parity::NONE) ? NOPARITY : 
                 (config.parity == Parity::ODD) ? ODDPARITY : EVENPARITY;
    
    if (!SetCommState(handle, &dcb)) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
        return false;
    }
    
    // 设置超时
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = config.readTimeout;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = config.writeTimeout;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    
    SetCommTimeouts(handle, &timeouts);
    
    return true;
#else
    // Linux/Unix实现
    fd = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        return false;
    }
    
    // 保存原始设置
    tcgetattr(fd, &oldTermios);
    
    // 配置串口
    struct termios options;
    tcgetattr(fd, &options);
    
    // 设置波特率
    speed_t baudRate = B115200;
    switch (config.baudRate) {
        case 9600:   baudRate = B9600; break;
        case 19200:  baudRate = B19200; break;
        case 38400:  baudRate = B38400; break;
        case 57600:  baudRate = B57600; break;
        case 115200: baudRate = B115200; break;
    }
    cfsetispeed(&options, baudRate);
    cfsetospeed(&options, baudRate);
    
    // 设置数据位、停止位、校验位
    options.c_cflag &= ~CSIZE;
    switch (static_cast<int>(config.dataBits)) {
        case 5: options.c_cflag |= CS5; break;
        case 6: options.c_cflag |= CS6; break;
        case 7: options.c_cflag |= CS7; break;
        case 8: options.c_cflag |= CS8; break;
    }
    
    // 应用设置
    tcsetattr(fd, TCSANOW, &options);
    
    return true;
#endif
}

void SerialInterface::Impl::close() {
#ifdef _WIN32
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
#else
    if (fd >= 0) {
        tcsetattr(fd, TCSANOW, &oldTermios);
        ::close(fd);
        fd = -1;
    }
#endif
}

bool SerialInterface::Impl::write(const std::vector<uint8_t>& data) {
#ifdef _WIN32
    DWORD written;
    return WriteFile(handle, data.data(), static_cast<DWORD>(data.size()), &written, NULL) && 
           written == data.size();
#else
    ssize_t written = ::write(fd, data.data(), data.size());
    return written == static_cast<ssize_t>(data.size());
#endif
}

std::vector<uint8_t> SerialInterface::Impl::read(size_t maxBytes, int timeoutMs) {
    std::vector<uint8_t> buffer(maxBytes);
    
#ifdef _WIN32
    DWORD bytesRead;
    if (ReadFile(handle, buffer.data(), static_cast<DWORD>(maxBytes), &bytesRead, NULL)) {
        buffer.resize(bytesRead);
        return buffer;
    }
#else
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(fd, &readSet);
    
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    if (select(fd + 1, &readSet, NULL, NULL, &timeout) > 0) {
        ssize_t bytesRead = ::read(fd, buffer.data(), maxBytes);
        if (bytesRead > 0) {
            buffer.resize(bytesRead);
            return buffer;
        }
    }
#endif
    
    return {};
}

int SerialInterface::Impl::bytesAvailable() const {
#ifdef _WIN32
    COMSTAT comstat;
    DWORD errors;
    if (ClearCommError(handle, &errors, &comstat)) {
        return comstat.cbInQue;
    }
    return 0;
#else
    int bytes = 0;
    ioctl(fd, FIONREAD, &bytes);
    return bytes;
#endif
}

void SerialInterface::Impl::flush() {
#ifdef _WIN32
    PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
#else
    tcflush(fd, TCIOFLUSH);
#endif
}