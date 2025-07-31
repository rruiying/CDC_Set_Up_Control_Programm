#include "utils/include/math_utils.h"
#include <algorithm>
#include <numeric>

float MathUtils::calculateAngle(float distance1, float distance2, float sensorSpacing) {
    if (sensorSpacing <= 0) return 0.0f;
    
    float heightDiff = distance2 - distance1;
    float angleRad = std::atan(heightDiff / sensorSpacing);
    return radiansToDegrees(angleRad);
}

float MathUtils::radiansToDegrees(float radians) {
    return radians * 180.0f / PI;
}

float MathUtils::degreesToRadians(float degrees) {
    return degrees * PI / 180.0f;
}

float MathUtils::calculateCapacitance(float area_mm2, float distance_mm, float dielectric) {
    if (distance_mm <= 0) return 0.0f;
    
    // C = ε₀ * εᵣ * A / d (结果为F)
    float capacitance_F = EPSILON_0 * dielectric * area_mm2 / distance_mm;
    
    // 转换为pF (1F = 1e12 pF)
    return capacitance_F * 1e12f;
}

float MathUtils::movingAverage(const std::vector<float>& data, int windowSize) {
    if (data.empty() || windowSize <= 0) return 0.0f;
    
    int actualWindow = std::min(windowSize, static_cast<int>(data.size()));
    float sum = 0.0f;
    
    // 取最后windowSize个数据
    for (int i = data.size() - actualWindow; i < data.size(); ++i) {
        sum += data[i];
    }
    
    return sum / actualWindow;
}

float MathUtils::exponentialSmooth(float currentValue, float newValue, float alpha) {
    alpha = clamp(alpha, 0.0f, 1.0f);
    return alpha * newValue + (1.0f - alpha) * currentValue;
}

float MathUtils::medianFilter(std::vector<float> data) {
    if (data.empty()) return 0.0f;
    
    std::sort(data.begin(), data.end());
    size_t n = data.size();
    
    if (n % 2 == 0) {
        return (data[n/2 - 1] + data[n/2]) / 2.0f;
    } else {
        return data[n/2];
    }
}

float MathUtils::clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

bool MathUtils::isApproximatelyEqual(float a, float b, float tolerance) {
    return std::abs(a - b) <= tolerance;
}

float MathUtils::lerp(float start, float end, float t) {
    t = clamp(t, 0.0f, 1.0f);
    return start + t * (end - start);
}

float MathUtils::mean(const std::vector<float>& data) {
    if (data.empty()) return 0.0f;
    
    float sum = std::accumulate(data.begin(), data.end(), 0.0f);
    return sum / data.size();
}

float MathUtils::standardDeviation(const std::vector<float>& data) {
    if (data.size() <= 1) return 0.0f;
    
    float avg = mean(data);
    float sum = 0.0f;
    
    for (float value : data) {
        float diff = value - avg;
        sum += diff * diff;
    }
    
    return std::sqrt(sum / (data.size() - 1));
}