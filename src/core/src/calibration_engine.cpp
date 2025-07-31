#include "core/include/calibration_engine.h"
#include "utils/include/logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <cmath>

CalibrationEngine::CalibrationEngine(QObject* parent)
    : QObject(parent)
{
    resetCalibration();
}

CalibrationEngine::~CalibrationEngine() {
}

void CalibrationEngine::performZeroCalibration(float currentHeight, float currentAngle) {
    emit calibrationStarted();
    
    m_data.heightZero = currentHeight;
    m_data.angleZero = currentAngle;
    m_data.calibrationTime = QDateTime::currentDateTime();
    
    LOG_INFO(QString("Zero calibration: H=%1mm, A=%2°")
            .arg(currentHeight).arg(currentAngle));
    
    emit calibrationCompleted();
}

bool CalibrationEngine::performMultiPointCalibration(const QList<CalibrationPoint>& points) {
    if (points.size() < 2) {
        emit calibrationFailed("需要至少2个校准点");
        return false;
    }
    
    emit calibrationStarted();
    
    m_data.heightPoints = points;
    
    // 计算线性拟合
    calculateLinearFit(points, m_data.heightOffset, m_data.heightScale);
    
    m_data.calibrationTime = QDateTime::currentDateTime();
    m_data.isValid = true;
    
    LOG_INFO(QString("Multi-point calibration completed. Offset=%1, Scale=%2")
            .arg(m_data.heightOffset).arg(m_data.heightScale));
    
    emit calibrationCompleted();
    return true;
}

void CalibrationEngine::performAutoCalibration() {
    emit calibrationStarted();
    
    // 自动校准逻辑
    // 这里需要与硬件配合，执行预定的校准序列
    
    emit calibrationProgress(25);
    // ... 执行校准步骤
    
    emit calibrationProgress(100);
    emit calibrationCompleted();
}

float CalibrationEngine::calibrateHeight(float rawHeight) const {
    if (m_data.heightPoints.size() >= 2) {
        // 使用多点校准数据
        return interpolate(rawHeight, m_data.heightPoints);
    } else {
        // 使用简单的线性校准
        return (rawHeight - m_data.heightZero) * m_data.heightScale + m_data.heightOffset;
    }
}

float CalibrationEngine::calibrateAngle(float rawAngle) const {
    return (rawAngle - m_data.angleZero) * m_data.angleScale + m_data.angleOffset;
}

float CalibrationEngine::temperatureCompensate(float value, float temperature) const {
    float tempDiff = temperature - m_data.refTemperature;
    return value + tempDiff * m_data.tempCoefficient;
}

void CalibrationEngine::setHeightOffset(float offset) {
    m_data.heightOffset = offset;
}

void CalibrationEngine::setHeightScale(float scale) {
    m_data.heightScale = scale;
}

void CalibrationEngine::setAngleOffset(float offset) {
    m_data.angleOffset = offset;
}

void CalibrationEngine::setAngleScale(float scale) {
    m_data.angleScale = scale;
}

void CalibrationEngine::setTemperatureCoefficient(float coeff) {
    m_data.tempCoefficient = coeff;
}

void CalibrationEngine::setReferenceTemperature(float temp) {
    m_data.refTemperature = temp;
}

void CalibrationEngine::setHeightReference(float measured, float actual) {
    CalibrationPoint point;
    point.measured = measured;
    point.actual = actual;
    
    // 查找是否已存在相同测量值的点
    auto it = std::find_if(m_data.heightPoints.begin(), 
                          m_data.heightPoints.end(),
                          [measured](const CalibrationPoint& p) {
                              return std::abs(p.measured - measured) < 0.001f;
                          });
    
    if (it != m_data.heightPoints.end()) {
        it->actual = actual;
    } else {
        m_data.heightPoints.append(point);
    }
    
    // 重新计算校准参数
    if (m_data.heightPoints.size() >= 2) {
        calculateLinearFit(m_data.heightPoints, 
                          m_data.heightOffset, 
                          m_data.heightScale);
    }
}

void CalibrationEngine::setAngleReference(float measured, float actual) {
    // 简单的偏移校准
    m_data.angleOffset = actual - measured;
}

bool CalibrationEngine::saveCalibration(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to save calibration: %1").arg(fileName));
        return false;
    }
    
    QJsonDocument doc(toJson());
    file.write(doc.toJson());
    file.close();
    
    LOG_INFO(QString("Calibration saved to: %1").arg(fileName));
    return true;
}

bool CalibrationEngine::loadCalibration(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to load calibration: %1").arg(fileName));
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    fromJson(doc.object());
    
    LOG_INFO(QString("Calibration loaded from: %1").arg(fileName));
    return true;
}

void CalibrationEngine::resetCalibration() {
    m_data = CalibrationData();
    m_data.heightOffset = 0.0f;
    m_data.heightScale = 1.0f;
    m_data.angleOffset = 0.0f;
    m_data.angleScale = 1.0f;
    m_data.tempCoefficient = 0.0f;
    m_data.refTemperature = 20.0f;
    m_data.heightZero = 0.0f;
    m_data.angleZero = 0.0f;
    m_data.isValid = false;
}

bool CalibrationEngine::isCalibrated() const {
    return m_data.isValid && m_data.calibrationTime.isValid();
}

bool CalibrationEngine::validateCalibration() const {
    // 检查校准参数是否合理
    if (std::abs(m_data.heightScale) < 0.1f || 
        std::abs(m_data.heightScale) > 10.0f) {
        return false;
    }
    
    if (std::abs(m_data.angleScale) < 0.1f || 
        std::abs(m_data.angleScale) > 10.0f) {
        return false;
    }
    
    return true;
}

void CalibrationEngine::calculateLinearFit(const QList<CalibrationPoint>& points,
                                          float& offset, float& scale) {
    if (points.size() < 2) {
        return;
    }
    
    // 最小二乘法线性拟合
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    int n = points.size();
    
    for (const auto& p : points) {
        sumX += p.measured;
        sumY += p.actual;
        sumXY += p.measured * p.actual;
        sumX2 += p.measured * p.measured;
    }
    
    scale = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    offset = (sumY - scale * sumX) / n;
}

float CalibrationEngine::interpolate(float value, 
                                   const QList<CalibrationPoint>& points) const {
    if (points.isEmpty()) {
        return value;
    }
    
    if (points.size() == 1) {
        return points[0].actual;
    }
    
    // 查找相邻的两个点
    for (int i = 0; i < points.size() - 1; ++i) {
        if (value >= points[i].measured && value <= points[i+1].measured) {
            // 线性插值
            float t = (value - points[i].measured) / 
                     (points[i+1].measured - points[i].measured);
            return points[i].actual + t * (points[i+1].actual - points[i].actual);
        }
    }
    
    // 超出范围，使用最近的点
    if (value < points.first().measured) {
        return points.first().actual;
    } else {
        return points.last().actual;
    }
}

QJsonObject CalibrationEngine::toJson() const {
    QJsonObject obj;
    
    obj["heightOffset"] = m_data.heightOffset;
    obj["heightScale"] = m_data.heightScale;
    obj["angleOffset"] = m_data.angleOffset;
    obj["angleScale"] = m_data.angleScale;
    obj["tempCoefficient"] = m_data.tempCoefficient;
    obj["refTemperature"] = m_data.refTemperature;
    obj["heightZero"] = m_data.heightZero;
    obj["angleZero"] = m_data.angleZero;
    obj["calibrationTime"] = m_data.calibrationTime.toString(Qt::ISODate);
    
    // 保存校准点
    QJsonArray pointsArray;
    for (const auto& p : m_data.heightPoints) {
        QJsonObject pointObj;
        pointObj["measured"] = p.measured;
        pointObj["actual"] = p.actual;
        pointsArray.append(pointObj);
    }
    obj["heightPoints"] = pointsArray;
    
    return obj;
}

void CalibrationEngine::fromJson(const QJsonObject& obj) {
    m_data.heightOffset = obj["heightOffset"].toDouble();
    m_data.heightScale = obj["heightScale"].toDouble();
    m_data.angleOffset = obj["angleOffset"].toDouble();
    m_data.angleScale = obj["angleScale"].toDouble();
    m_data.tempCoefficient = obj["tempCoefficient"].toDouble();
    m_data.refTemperature = obj["refTemperature"].toDouble();
    m_data.heightZero = obj["heightZero"].toDouble();
    m_data.angleZero = obj["angleZero"].toDouble();
    
    QString timeStr = obj["calibrationTime"].toString();
    m_data.calibrationTime = QDateTime::fromString(timeStr, Qt::ISODate);
    
    // 加载校准点
    m_data.heightPoints.clear();
    QJsonArray pointsArray = obj["heightPoints"].toArray();
    for (const auto& value : pointsArray) {
        QJsonObject pointObj = value.toObject();
        CalibrationPoint point;
        point.measured = pointObj["measured"].toDouble();
        point.actual = pointObj["actual"].toDouble();
        m_data.heightPoints.append(point);
    }
    
    m_data.isValid = true;
}