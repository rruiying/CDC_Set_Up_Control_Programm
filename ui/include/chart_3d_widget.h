// ui/include/chart_3d_widget.h
#ifndef CHART_3D_WIDGET_H
#define CHART_3D_WIDGET_H

#include <QWidget>
#include <memory>
#include <vector>
#include "../../src/data/include/csv_analyzer.h"

#ifdef USE_QT3D
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DRender/QCamera>

namespace Qt3DCore {
    class QTransform;
}
namespace Qt3DExtras {
    class QPhongMaterial;
    class QSphereMesh;
    class QCylinderMesh;
    class QOrbitCameraController;
}
namespace Qt3DRender {
    class QPointLight;
}
#endif

// 3D数据点
struct Point3D {
    float x, y, z;
    float value;  // 用于颜色映射（温度）
    
    Point3D(float x = 0, float y = 0, float z = 0, float v = 0) 
        : x(x), y(y), z(z), value(v) {}
};

// 3D图表配置
struct Chart3DConfig {
    // 轴标签
    QString xLabel = "距离 (mm)";
    QString yLabel = "角度 (°)";
    QString zLabel = "电容 (pF)";
    QString colorLabel = "温度 (°C)";
    
    // 显示选项
    bool showGrid = true;
    bool showAxes = true;
    bool showColorBar = true;
    bool enableRotation = true;
    bool enableZoom = true;
    
    // 点样式
    float pointSize = 2.0f;
    bool connectPoints = true;  // 是否连接点成面
    
    // 颜色映射范围
    float colorMin = 20.0f;  // 最小温度
    float colorMax = 30.0f;  // 最大温度
};

class Chart3DWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit Chart3DWidget(QWidget *parent = nullptr);
    ~Chart3DWidget();
    
    // ===== 数据管理 =====
    
    // 设置3D数据点
    void setData(const std::vector<Point3D>& points);
    void setData(const std::vector<DataPoint>& dataPoints);
    void setData(const Data3D& data3d);
    
    // 清空数据
    void clearData();
    
    // 获取数据信息
    size_t getPointCount() const;
    
    // ===== 配置 =====
    
    // 设置配置
    void setConfig(const Chart3DConfig& config);
    Chart3DConfig getConfig() const;
    
    // ===== 视图控制 =====
    
    // 重置视图
    void resetView();
    
    // 设置相机位置
    void setCameraPosition(float x, float y, float z);
    void setCameraLookAt(float x, float y, float z);
    
    // 旋转视图
    void rotateView(float dx, float dy);
    
    // 缩放
    void zoomIn();
    void zoomOut();
    
    // ===== 导出 =====
    
    // 截图
    bool captureImage(const QString& filename);
    
    // ===== 交互模式 =====
    
    enum class InteractionMode {
        ROTATE,
        PAN,
        ZOOM,
        SELECT
    };
    
    void setInteractionMode(InteractionMode mode);
    
signals:
    // 点被选中
    void pointSelected(float x, float y, float z, float value);
    
    // 视图改变
    void viewChanged();
    
protected:
    void resizeEvent(QResizeEvent *event) override;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
#ifdef USE_QT3D
    // Qt3D组件
    Qt3DExtras::Qt3DWindow *view3D;
    QWidget *container;
    Qt3DCore::QEntity *rootEntity;
    Qt3DRender::QCamera *camera;
    Qt3DExtras::QOrbitCameraController *cameraController;
    
    // 场景实体
    std::vector<Qt3DCore::QEntity*> dataPoints;
    Qt3DCore::QEntity *axesEntity;
    Qt3DCore::QEntity *gridEntity;
    
    // 私有方法
    void initializeQt3D();
    void createScene();
    void createAxes();
    void createGrid();
    void createDataPoints();
    void updateDataPoints();
    
    // 颜色映射
    QColor valueToColor(float value) const;
    
    // 创建3D对象
    Qt3DCore::QEntity* createSphere(const QVector3D& position, float radius, const QColor& color);
    Qt3DCore::QEntity* createCylinder(const QVector3D& start, const QVector3D& end, 
                                      float radius, const QColor& color);
    Qt3DCore::QEntity* createLine(const QVector3D& start, const QVector3D& end, const QColor& color);
    
#else
    // 如果没有Qt3D，使用OpenGL或2D投影
    void initializeFallback();
    void paint2DProjection(QPainter* painter);
#endif
    
    // 当前数据
    std::vector<Point3D> currentData;
    Chart3DConfig config;
    
    // 数据范围
    struct DataRange {
        float xMin, xMax;
        float yMin, yMax;
        float zMin, zMax;
        float valueMin, valueMax;
    } dataRange;
    
    // 计算数据范围
    void calculateDataRange();
    
    // 归一化坐标
    QVector3D normalizePoint(const Point3D& point) const;
};

// ===== 简化的3D视图（使用2D投影） =====

class Simple3DView : public QWidget {
    Q_OBJECT
    
public:
    explicit Simple3DView(QWidget *parent = nullptr);
    
    // 设置数据
    void setData(const std::vector<Point3D>& points);
    
    // 设置旋转角度
    void setRotation(float angleX, float angleY, float angleZ);
    
    // 设置缩放
    void setZoom(float zoom);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    
private:
    std::vector<Point3D> data;
    
    // 视图参数
    float rotationX = 30.0f;
    float rotationY = 45.0f;
    float rotationZ = 0.0f;
    float zoomFactor = 1.0f;
    
    // 鼠标交互
    QPoint lastMousePos;
    bool isRotating = false;
    
    // 投影矩阵
    struct Matrix4x4 {
        float m[4][4];
        Matrix4x4();
        static Matrix4x4 rotationX(float angle);
        static Matrix4x4 rotationY(float angle);
        static Matrix4x4 rotationZ(float angle);
        static Matrix4x4 scale(float s);
        static Matrix4x4 translation(float x, float y, float z);
        Matrix4x4 operator*(const Matrix4x4& other) const;
        QPointF project(const Point3D& point) const;
    };
    
    // 将3D点投影到2D
    QPointF projectTo2D(const Point3D& point) const;
    
    // 绘制
    void drawAxes(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawDataPoints(QPainter& painter);
    void drawColorBar(QPainter& painter);
    
    // 颜色映射
    QColor temperatureToColor(float temp) const;
};

#endif // CHART_3D_WIDGET_H