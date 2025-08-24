#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <chrono>
#include <string>
#include <cstdint>

class TimeUtils {
public:
    enum class TimeZone {
        LOCAL,
        UTC
    };

    static int64_t getCurrentTimestamp();
    
    static std::string formatTimestamp(int64_t timestamp, TimeZone tz = TimeZone::LOCAL);

    static std::string toISO8601(int64_t timestamp);
};

#endif