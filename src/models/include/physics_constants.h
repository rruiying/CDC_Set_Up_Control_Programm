#ifndef PHYSICS_CONSTANTS_H
#define PHYSICS_CONSTANTS_H

namespace PhysicsConstants {
    // 基础物理常量
    constexpr double PI = 3.14159265358979323846;
    constexpr double EPSILON_0 = 8.854187817e-12; // 真空介电常数 (F/m)
    
    // 角度转换
    constexpr double RAD_TO_DEG = 180.0 / PI;
    constexpr double DEG_TO_RAD = PI / 180.0;
    
    // 传感器范围
    constexpr double DISTANCE_SENSOR_MIN = 0.0;
    constexpr double DISTANCE_SENSOR_MAX = 300.0;
    constexpr double TEMPERATURE_MIN = -40.0;
    constexpr double TEMPERATURE_MAX = 85.0;
    constexpr double ANGLE_MIN = -90.0;
    constexpr double ANGLE_MAX = 90.0;
    constexpr double CAPACITANCE_MIN = 0.0;
    constexpr double CAPACITANCE_MAX = 1000.0;
}

#endif