#include "utils/include/math_utils.h"
#include <algorithm>
#include <numeric>

double MathUtils::calculateAngleFromSensors(double distance1, double distance2, double sensorSpacing) {
    if (sensorSpacing <= 0) return 0.0;
    
    double heightDiff = distance2 - distance1;
    double angleRad = std::atan(heightDiff / sensorSpacing);
    return radiansToDegrees(angleRad);
}

double MathUtils::radiansToDegrees(double radians) {
    return radians * 180.0 / PI;
}

double MathUtils::degreesToRadians(double degrees) {
    return degrees * PI / 180.0;
}

double MathUtils::calculateParallelPlateCapacitance(double plateArea_mm2, double distance_mm, double dielectricConstant, double angle_degrees) {
    if (distance_mm <= 0) return 0.0;
    
    // 计算角度影响下的有效面积
    double angleRad = degreesToRadians(angle_degrees);
    double effectiveArea = plateArea_mm2 * std::cos(angleRad);

    // 转换单位
    double area_m2 = effectiveArea * 1e-6;  // mm² to m²
    double distance_m = distance_mm * 1e-3;  // mm to m

    // C = ε₀ * εᵣ * A / d
    double capacitance_F = EPSILON_0 * dielectricConstant * area_m2 / distance_m;

    // 转换为pF
    return capacitance_F * 1e12;
}

double MathUtils::movingAverage(const std::vector<double>& data, int windowSize) {
    if (data.empty() || windowSize <= 0) return 0.0f;
    
    int actualWindow = std::min(windowSize, static_cast<int>(data.size()));
    double sum = 0.0;
    
    // 取最后windowSize个数据
    for (int i = data.size() - actualWindow; i < data.size(); ++i) {
        sum += data[i];
    }
    
    return sum / actualWindow;
}

double MathUtils::exponentialSmooth(double currentValue, double newValue, double alpha) {
    alpha = clamp(alpha, 0.0, 1.0);
    return alpha * newValue + (1.0 - alpha) * currentValue;
}

double MathUtils::medianFilter(std::vector<double> data) {
    if (data.empty()) return 0.0;
    
    std::sort(data.begin(), data.end());
    size_t n = data.size();
    
    if (n % 2 == 0) {
        return (data[n/2 - 1] + data[n/2]) / 2.0;
    } else {
        return data[n/2];
    }
}

double MathUtils::clamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

bool MathUtils::isApproximatelyEqual(double a, double b, double tolerance) {
    return std::abs(a - b) <= tolerance;
}

double MathUtils::lerp(double start, double end, double t) {
    t = clamp(t, 0.0, 1.0);
    return start + t * (end - start);
}

double MathUtils::mean(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    
    double sum = std::accumulate(data.begin(), data.end(), 0.0f);
    return sum / data.size();
}

double MathUtils::standardDeviation(const std::vector<double>& data) {
    if (data.size() <= 1) return 0.0;
    
    double avg = mean(data);
    double sum = 0.0;
    
    for (double value : data) {
        double diff = value - avg;
        sum += diff * diff;
    }
    
    return std::sqrt(sum / (data.size() - 1));
}

double MathUtils::calculateEffectiveArea(double originalArea_mm2, double angle_degrees) {
    double angleRad = degreesToRadians(angle_degrees);
    return originalArea_mm2 * std::cos(angleRad);
}

bool MathUtils::isInRange(double value, double min, double max) {
    return value >= min && value <= max;
}

void MathUtils::minMax(const std::vector<double>& data, double& min, double& max) {
    if (data.empty()) {
        min = max = 0.0;
        return;
    }
    
    min = *std::min_element(data.begin(), data.end());
    max = *std::max_element(data.begin(), data.end());
}