#include "../include/physics_calculator.h"
#include "../include/physics_constants.h"
#include <cmath>

double PhysicsCalculator::calculateParallelPlateCapacitance(
    double plateArea_mm2, double distance_mm, 
    double angle_degrees, double dielectricConstant) {
    
    if (distance_mm <= 0.0) return 0.0;
    
    double angleRad = angle_degrees * PhysicsConstants::DEG_TO_RAD;
    double effectiveArea_mm2 = plateArea_mm2 * std::cos(angleRad);
    double area_m2 = effectiveArea_mm2 * 1e-6;
    double distance_m = distance_mm * 1e-3;
    
    double capacitance_F = PhysicsConstants::EPSILON_0 * 
                          dielectricConstant * area_m2 / distance_m;
    return capacitance_F * 1e12; // 转换为pF
}

double PhysicsCalculator::calculateAngleFromSensors(
    double distance1, double distance2, double sensorSpacing) {
    
    if (sensorSpacing <= 0.0) return 0.0;
    double heightDiff = distance2 - distance1;
    double angleRad = std::atan(heightDiff / sensorSpacing);
    return angleRad * PhysicsConstants::RAD_TO_DEG;
}