#ifndef PHYSICS_CALCULATOR_H
#define PHYSICS_CALCULATOR_H

class PhysicsCalculator {
public:
    // 计算平行板电容（返回pF）
    static double calculateParallelPlateCapacitance(
        double plateArea_mm2,
        double distance_mm,
        double angle_degrees = 0.0,
        double dielectricConstant = 1.0);
    
    // 从两个传感器计算角度
    static double calculateAngleFromSensors(
        double distance1,
        double distance2,
        double sensorSpacing);
};

#endif