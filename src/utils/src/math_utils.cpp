#include "../include/math_utils.h"
#include "../include/statistics_utils.h"
#include <algorithm>
#include <numeric>

double MathUtils::movingAverage(const std::vector<double>& data, int windowSize) {
    if (data.empty() || windowSize <= 0) return 0.0;
    
    int n = static_cast<int>(data.size());
    int w = std::min(windowSize, n);
    
    // 提取窗口数据
    std::vector<double> window(data.end() - w, data.end());
    
    // 复用StatisticsUtils::mean
    return StatisticsUtils::mean(window);
}

double MathUtils::exponentialSmooth(double currentValue, double newValue, double alpha) {
    alpha = clamp(alpha, 0.0, 1.0);
    return alpha * newValue + (1.0 - alpha) * currentValue;
}

double MathUtils::medianFilter(std::vector<double> window) {
    // 复用StatisticsUtils::median
    return StatisticsUtils::median(window);
}

double MathUtils::mapRange(double x, double in_min, double in_max, 
                          double out_min, double out_max) {
    if (in_min == in_max) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool MathUtils::isInRange(double value, double min, double max) {
    return value >= min && value <= max;
}

void MathUtils::minMax(const std::vector<double>& data, double& min, double& max) {
    if (data.empty()) {
        min = max = 0.0;
        return;
    }
    auto minmax = std::minmax_element(data.begin(), data.end());
    min = *minmax.first;
    max = *minmax.second;
}