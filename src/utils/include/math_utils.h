#pragma once
#include <vector>
#include <cmath>

class MathUtils {
public:
    // 角度计算
    static float calculateAngle(float distance1, float distance2, float sensorSpacing);
    static float radiansToDegrees(float radians);
    static float degreesToRadians(float degrees);
    
    // 电容计算 (返回pF)
    static float calculateCapacitance(float area_mm2, float distance_mm, 
                                    float dielectric = 1.0f);
    
    // 数据滤波
    static float movingAverage(const std::vector<float>& data, int windowSize);
    static float exponentialSmooth(float currentValue, float newValue, float alpha);
    static float medianFilter(std::vector<float> data);  // 注意：传值不传引用
    
    // 工具函数
    static float clamp(float value, float min, float max);
    static bool isApproximatelyEqual(float a, float b, float tolerance = 0.001f);
    static float lerp(float start, float end, float t);  // 线性插值
    
    // 统计函数
    static float mean(const std::vector<float>& data);
    static float standardDeviation(const std::vector<float>& data);
    
private:
    static constexpr float EPSILON_0 = 8.854e-15f;  // F/mm (真空介电常数)
    static constexpr float PI = 3.14159265359f;
};