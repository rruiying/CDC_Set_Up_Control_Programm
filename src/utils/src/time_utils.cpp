#include "../include/time_utils.h"
#include <sstream>
#include <iomanip>
#include <ctime>

int64_t TimeUtils::getCurrentTimestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<milliseconds>(duration).count();
}

std::string TimeUtils::formatTimestamp(int64_t timestamp, TimeZone tz) {
    auto seconds = timestamp / 1000;
    std::time_t time = static_cast<std::time_t>(seconds);
    std::tm tm{};

    if (tz == TimeZone::LOCAL) {
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    } else {
#ifdef _WIN32
        gmtime_s(&tm, &time);
#else
        gmtime_r(&time, &tm);
#endif
    }

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    // 添加毫秒
    int milliseconds = timestamp % 1000;
    oss << "." << std::setfill('0') << std::setw(3) << milliseconds;

    if (tz == TimeZone::UTC) {
        oss << "Z";
    }
    
    return oss.str();
}

std::string TimeUtils::toISO8601(int64_t timestamp) {
    auto seconds = timestamp / 1000;
    std::time_t time = static_cast<std::time_t>(seconds);
    std::tm tm{};
    
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    // 添加毫秒和时区
    int milliseconds = timestamp % 1000;
    oss << "." << std::setfill('0') << std::setw(3) << milliseconds << "Z";
    
    return oss.str();
}