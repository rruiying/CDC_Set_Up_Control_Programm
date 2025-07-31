#include <gtest/gtest.h>
#include "utils/include/math_utils.h"
#include <cmath>

class MathUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试角度计算
TEST_F(MathUtilsTest, CalculateAngleFromDistances) {
    // 两个传感器相距100mm，高度差5mm
    float distance1 = 25.0f;
    float distance2 = 30.0f;
    float sensorSpacing = 100.0f;
    
    float angle = MathUtils::calculateAngle(distance1, distance2, sensorSpacing);
    
    // arctan(5/100) ≈ 2.86度
    EXPECT_NEAR(angle, 2.86f, 0.1f);
}

// 测试角度为0的情况
TEST_F(MathUtilsTest, CalculateZeroAngle) {
    float angle = MathUtils::calculateAngle(25.0f, 25.0f, 100.0f);
    EXPECT_FLOAT_EQ(angle, 0.0f);
}

// 测试弧度角度转换
TEST_F(MathUtilsTest, AngleConversion) {
    EXPECT_NEAR(MathUtils::radiansToDegrees(M_PI), 180.0f, 0.001f);
    EXPECT_NEAR(MathUtils::degreesToRadians(180.0f), M_PI, 0.001f);
    EXPECT_NEAR(MathUtils::radiansToDegrees(M_PI/4), 45.0f, 0.001f);
}

// 测试平行板电容计算
TEST_F(MathUtilsTest, CalculateCapacitance) {
    float area = 100.0f;      // 100 mm²
    float distance = 1.0f;    // 1 mm
    float epsilon_r = 1.0f;   // 空气
    
    float capacitance = MathUtils::calculateCapacitance(area, distance, epsilon_r);
    
    // C = ε₀ * εᵣ * A / d
    // ε₀ = 8.854e-12 F/m = 8.854e-15 F/mm
    // C = 8.854e-15 * 1 * 100 / 1 = 8.854e-13 F = 0.8854 pF
    EXPECT_NEAR(capacitance, 0.8854f, 0.01f);
}

// 测试移动平均滤波
TEST_F(MathUtilsTest, MovingAverage) {
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    float avg = MathUtils::movingAverage(data, 3);
    
    // 最后3个值的平均: (3+4+5)/3 = 4
    EXPECT_FLOAT_EQ(avg, 4.0f);
}

// 测试数据平滑
TEST_F(MathUtilsTest, ExponentialSmoothing) {
    float currentValue = 10.0f;
    float newValue = 20.0f;
    float alpha = 0.3f;
    
    float smoothed = MathUtils::exponentialSmooth(currentValue, newValue, alpha);
    
    // smoothed = alpha * new + (1-alpha) * current
    // = 0.3 * 20 + 0.7 * 10 = 6 + 7 = 13
    EXPECT_FLOAT_EQ(smoothed, 13.0f);
}

// 测试范围限制
TEST_F(MathUtilsTest, Clamp) {
    EXPECT_FLOAT_EQ(MathUtils::clamp(50.0f, 0.0f, 100.0f), 50.0f);
    EXPECT_FLOAT_EQ(MathUtils::clamp(-10.0f, 0.0f, 100.0f), 0.0f);
    EXPECT_FLOAT_EQ(MathUtils::clamp(150.0f, 0.0f, 100.0f), 100.0f);
}