#include <gtest/gtest.h>
#include <QSignalSpy>
#include "core/include/calibration_engine.h"

class CalibrationEngineTest : public ::testing::Test {
protected:
    CalibrationEngine* calibration;
    
    void SetUp() override {
        calibration = new CalibrationEngine();
    }
    
    void TearDown() override {
        delete calibration;
    }
};

// 测试高度校准
TEST_F(CalibrationEngineTest, HeightCalibration) {
    // 设置参考点
    calibration->setHeightReference(0.0f, 0.0f);    // 0mm读数对应0mm实际
    calibration->setHeightReference(100.0f, 95.0f); // 100mm读数对应95mm实际
    
    // 测试校准
    float calibrated = calibration->calibrateHeight(50.0f);
    EXPECT_NEAR(calibrated, 47.5f, 0.1f); // 线性插值
}

// 测试角度校准
TEST_F(CalibrationEngineTest, AngleCalibration) {
    calibration->setAngleOffset(2.5f); // 2.5度偏移
    
    float calibrated = calibration->calibrateAngle(10.0f);
    EXPECT_FLOAT_EQ(calibrated, 12.5f);
}

// 测试零点校准
TEST_F(CalibrationEngineTest, ZeroCalibration) {
    QSignalSpy spy(calibration, &CalibrationEngine::calibrationCompleted);
    
    // 执行零点校准
    calibration->performZeroCalibration(25.0f, 2.0f);
    
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FLOAT_EQ(calibration->getHeightZero(), 25.0f);
    EXPECT_FLOAT_EQ(calibration->getAngleZero(), 2.0f);
}

// 测试多点校准
TEST_F(CalibrationEngineTest, MultiPointCalibration) {
    CalibrationEngine::CalibrationPoint p1{0.0f, 0.0f};
    CalibrationEngine::CalibrationPoint p2{50.0f, 48.0f};
    CalibrationEngine::CalibrationPoint p3{100.0f, 95.0f};
    
    QList<CalibrationEngine::CalibrationPoint> points = {p1, p2, p3};
    
    bool result = calibration->performMultiPointCalibration(points);
    EXPECT_TRUE(result);
    
    // 测试校准效果
    float calibrated = calibration->calibrateHeight(75.0f);
    EXPECT_NEAR(calibrated, 71.5f, 1.0f); // 大致线性关系
}

// 测试保存和加载校准数据
TEST_F(CalibrationEngineTest, SaveLoadCalibration) {
    // 设置一些校准数据
    calibration->setHeightOffset(5.0f);
    calibration->setAngleOffset(2.5f);
    
    // 保存
    QString testFile = "test_calibration.json";
    bool saved = calibration->saveCalibration(testFile);
    EXPECT_TRUE(saved);
    
    // 创建新实例并加载
    CalibrationEngine newCalib;
    bool loaded = newCalib.loadCalibration(testFile);
    EXPECT_TRUE(loaded);
    
    // 验证数据
    EXPECT_FLOAT_EQ(newCalib.getHeightOffset(), 5.0f);
    EXPECT_FLOAT_EQ(newCalib.getAngleOffset(), 2.5f);
    
    // 清理测试文件
    QFile::remove(testFile);
}

// 测试温度补偿
TEST_F(CalibrationEngineTest, TemperatureCompensation) {
    calibration->setTemperatureCoefficient(0.1f); // 0.1mm/°C
    calibration->setReferenceTemperature(20.0f);
    
    float compensated = calibration->temperatureCompensate(100.0f, 25.0f);
    // 100mm + (25-20) * 0.1 = 100.5mm
    EXPECT_FLOAT_EQ(compensated, 100.5f);
}