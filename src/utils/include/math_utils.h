#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <vector>

class MathUtils {
public:
    static constexpr double DEFAULT_DIELECTRIC_CONSTANT = 1.0;
    static constexpr double DEFAULT_PLATE_AREA_MM2 = 400.0;
    static constexpr double DEFAULT_SYSTEM_HEIGHT_MM = 50.0;
    static constexpr double DEFAULT_MIN_HEIGHT_MM = 0.0;
    static constexpr double DEFAULT_MAX_HEIGHT_MM = 150.0;
    static constexpr double DEFAULT_MIN_ANGLE_DEG = -90.0;
    static constexpr double DEFAULT_MAX_ANGLE_DEG = 90.0;

    static constexpr double clamp(double v, double lo, double hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

    static double movingAverage(const std::vector<double>& data, int windowSize);
    static double exponentialSmooth(double currentValue, double newValue, double alpha);
    static double medianFilter(std::vector<double> window);
    static double mapRange(double x, double in_min, double in_max, double out_min, double out_max);
    static bool isInRange(double value, double min, double max);
    static void minMax(const std::vector<double>& data, double& min, double& max);
};

#endif // MATH_UTILS_H
