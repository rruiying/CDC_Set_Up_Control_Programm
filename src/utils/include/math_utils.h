// src/utils/include/math_utils.h
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <vector>
#include <cmath>

/**
 * @brief 数学工具类
 * 
 * 提供CDC系统需要的各种数学计算功能：
 * - 电容计算
 * - 角度计算
 * - 数据滤波
 * - 统计分析
 */
class MathUtils {
public:
    // ===== 电容计算 =====
    
    /**
     * @brief 计算平行板电容（考虑角度影响）
     * @param plateArea_mm2 板面积（平方毫米）
     * @param distance_mm 板间距离（毫米）
     * @param angle_degrees 倾斜角度（度，0表示水平）
     * @param dielectricConstant 相对介电常数
     * @return 电容值（pF）
     */
    static double calculateParallelPlateCapacitance(
        double plateArea_mm2,
        double distance_mm,
        double angle_degrees = 0.0,
        double dielectricConstant = 1.0
    );
    
    /**
     * @brief 计算角度影响下的有效面积
     * @param originalArea_mm2 原始板面积
     * @param angle_degrees 倾斜角度
     * @return 有效面积（平方毫米）
     */
    static double calculateEffectiveArea(double originalArea_mm2, double angle_degrees);
    
    // ===== 角度计算 =====
    
    /**
     * @brief 从两个距离传感器计算角度
     * @param sensor1_mm 传感器1的距离读数
     * @param sensor2_mm 传感器2的距离读数
     * @param sensorSpacing_mm 两传感器间距
     * @return 计算得到的角度（度）
     */
    static double calculateAngleFromSensors(
        double sensor1_mm, 
        double sensor2_mm, 
        double sensorSpacing_mm
    );
    
    /**
     * @brief 角度转换
     */
    static double radiansToDegrees(double radians);
    static double degreesToRadians(double degrees);
    
    // ===== 数据滤波 =====
    
    /**
     * @brief 移动平均滤波
     * @param data 数据序列
     * @param windowSize 窗口大小
     * @return 滤波后的值
     */
    static double movingAverage(const std::vector<double>& data, int windowSize);
    
    /**
     * @brief 指数平滑滤波
     * @param currentValue 当前值
     * @param newValue 新值
     * @param alpha 平滑系数（0-1）
     * @return 平滑后的值
     */
    static double exponentialSmooth(double currentValue, double newValue, double alpha);
    
    /**
     * @brief 中值滤波
     * @param data 数据序列（会被排序）
     * @return 中值
     */
    static double medianFilter(std::vector<double> data);
    
    // ===== 数值处理工具 =====
    
    /**
     * @brief 数值范围限制
     * @param value 输入值
     * @param min 最小值
     * @param max 最大值
     * @return 限制后的值
     */
    static double clamp(double value, double min, double max);
    
    /**
     * @brief 检查数值是否在指定范围内
     * @param value 待检查的值
     * @param min 最小值
     * @param max 最大值
     * @return true 如果在范围内
     */
    static bool isInRange(double value, double min, double max);
    
    /**
     * @brief 近似相等比较
     * @param a 值A
     * @param b 值B
     * @param tolerance 容差
     * @return true 如果近似相等
     */
    static bool isApproximatelyEqual(double a, double b, double tolerance = 0.001);
    
    /**
     * @brief 线性插值
     * @param start 起始值
     * @param end 结束值
     * @param t 插值参数（0-1）
     * @return 插值结果
     */
    static double lerp(double start, double end, double t);
    
    // ===== 统计函数 =====
    
    /**
     * @brief 计算平均值
     * @param data 数据序列
     * @return 平均值
     */
    static double mean(const std::vector<double>& data);
    
    /**
     * @brief 计算标准差
     * @param data 数据序列
     * @return 标准差
     */
    static double standardDeviation(const std::vector<double>& data);
    
    /**
     * @brief 计算最小值和最大值
     * @param data 数据序列
     * @param min 输出最小值
     * @param max 输出最大值
     */
    static void minMax(const std::vector<double>& data, double& min, double& max);
    
    // ===== 常量定义 =====
    
    // 物理常量
    static constexpr double EPSILON_0 = 8.854e-12;    // F/m (真空介电常数)
    static constexpr double PI = 3.14159265359;       // 圆周率
    
    // 默认系统参数（可通过配置文件修改）
    static constexpr double DEFAULT_PLATE_AREA_MM2 = 2500.0;      // 默认板面积 50mm x 50mm
    static constexpr double DEFAULT_DIELECTRIC_CONSTANT = 1.0;    // 默认介电常数（空气）
    static constexpr double DEFAULT_SENSOR_SPACING_MM = 80.0;     // 默认传感器间距
    static constexpr double DEFAULT_SYSTEM_HEIGHT_MM = 180.0;     // 默认系统高度
    static constexpr double DEFAULT_MIDDLE_PLATE_HEIGHT_MM = 25.0; // 默认中间板高度
    
    // 默认安全限位
    static constexpr double DEFAULT_MIN_HEIGHT_MM = 0.0;
    static constexpr double DEFAULT_MAX_HEIGHT_MM = 180.0;
    static constexpr double DEFAULT_MIN_ANGLE_DEG = -90.0;
    static constexpr double DEFAULT_MAX_ANGLE_DEG = 90.0;

private:
    // 私有辅助方法
    static double convertMmToMeters(double mm) { return mm / 1000.0; }
    static double convertM2ToMm2(double m2) { return m2 * 1e6; }
    static double convertFToPF(double f) { return f * 1e12; }
};

#endif // MATH_UTILS_H