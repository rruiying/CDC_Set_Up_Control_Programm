#pragma once
#include <QObject>
#include <QList>
#include <QPointF>
#include <QJsonObject>

class CalibrationEngine : public QObject {
    Q_OBJECT
    
public:
    struct CalibrationPoint {
        float measured;
        float actual;
    };
    
    struct CalibrationData {
        // 高度校准
        float heightOffset;
        float heightScale;
        QList<CalibrationPoint> heightPoints;
        
        // 角度校准
        float angleOffset;
        float angleScale;
        
        // 温度补偿
        float tempCoefficient;
        float refTemperature;
        
        // 零点
        float heightZero;
        float angleZero;
        
        // 校准时间
        QDateTime calibrationTime;
        bool isValid;
    };
    
    explicit CalibrationEngine(QObject* parent = nullptr);
    ~CalibrationEngine();
    
    // 校准方法
    void performZeroCalibration(float currentHeight, float currentAngle);
    bool performMultiPointCalibration(const QList<CalibrationPoint>& points);
    void performAutoCalibration();
    
    // 应用校准
    float calibrateHeight(float rawHeight) const;
    float calibrateAngle(float rawAngle) const;
    float temperatureCompensate(float value, float temperature) const;
    
    // 设置校准参数
    void setHeightOffset(float offset);
    void setHeightScale(float scale);
    void setAngleOffset(float offset);
    void setAngleScale(float scale);
    void setTemperatureCoefficient(float coeff);
    void setReferenceTemperature(float temp);
    
    // 参考点设置
    void setHeightReference(float measured, float actual);
    void setAngleReference(float measured, float actual);
    
    // 获取校准参数
    float getHeightOffset() const { return m_data.heightOffset; }
    float getHeightScale() const { return m_data.heightScale; }
    float getAngleOffset() const { return m_data.angleOffset; }
    float getAngleScale() const { return m_data.angleScale; }
    float getHeightZero() const { return m_data.heightZero; }
    float getAngleZero() const { return m_data.angleZero; }
    
    // 校准数据管理
    bool saveCalibration(const QString& fileName);
    bool loadCalibration(const QString& fileName);
    void resetCalibration();
    CalibrationData getCalibrationData() const { return m_data; }
    
    // 验证
    bool isCalibrated() const;
    bool validateCalibration() const;
    
signals:
    void calibrationStarted();
    void calibrationCompleted();
    void calibrationFailed(const QString& reason);
    void calibrationProgress(int percent);
    
private:
    CalibrationData m_data;
    
    // 内部计算方法
    void calculateLinearFit(const QList<CalibrationPoint>& points,
                           float& offset, float& scale);
    float interpolate(float value, const QList<CalibrationPoint>& points) const;
    
    // JSON序列化
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
};