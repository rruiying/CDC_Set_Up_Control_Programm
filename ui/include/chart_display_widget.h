// ui/include/chart_display_widget.h
#ifndef CHART_DISPLAY_WIDGET_H
#define CHART_DISPLAY_WIDGET_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <memory>
#include <functional>
#include "../../src/data/include/csv_analyzer.h"

// 前向声明
class QCustomPlot;
class QCPGraph;
class QCPColorMap;
class QCPScatterStyle;
class QCPCurve;

// 图表类型枚举
enum class ChartType {
    DISTANCE_CAPACITANCE,           // 距离-电容分析
    ANGLE_CAPACITANCE,             // 角度-电容分析
    DISTANCE_ANGLE_CAPACITANCE_3D, // 3D图表
    TEMPERATURE_CAPACITANCE,       // 温度-电容分析
    ERROR_ANALYSIS                 // 误差分析
};

// 数据源类型
enum class DataSource {
    CSV_FILE,     // CSV文件
    REAL_TIME     // 实时数据
};

// 图表范围
struct ChartRange {
    double xMin, xMax;
    double yMin, yMax;
    double zMin, zMax;  // 用于3D
};

// 图表配置
struct ChartConfig {
    bool showGrid = true;           // 显示网格
    bool autoScale = true;          // 自动缩放
    bool showLegend = true;         // 显示图例
    bool showDataPoints = true;     // 显示数据点
    bool showTrendLine = true;      // 显示趋势线
    bool enableZoom = true;         // 启用缩放
    bool enablePan = true;          // 启用平移
    
    // 样式设置
    QColor lineColor = Qt::blue;
    QColor pointColor = Qt::red;
    QColor gridColor = Qt::lightGray;
    int lineWidth = 2;
    int pointSize = 6;
    
    // 轴标签
    QString xLabel;
    QString yLabel;
    QString title;
};

// 3D数据结构
struct Data3D {
    std::vector<double> xPoints;
    std::vector<double> yPoints;
    std::vector<double> zPoints;
    std::vector<double> colorValues; // 温度值用于颜色映射
};

class ChartDisplayWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ChartDisplayWidget(QWidget *parent = nullptr);
    ~ChartDisplayWidget();
    
    // ===== 图表类型与数据 =====
    
    // 设置图表类型
    void setChartType(ChartType type);
    ChartType getChartType() const;
    
    // 设置数据
    void setData(const std::vector<MeasurementData>& data);
    void setDataPoints(const std::vector<DataPoint>& points);
    
    // 添加单个数据点（用于实时更新）
    void addDataPoint(const MeasurementData& data);
    void addDataPoint(const DataPoint& point);
    
    // 清空数据
    void clearData();
    
    // 获取数据信息
    int getDataPointCount() const;
    ChartRange getDataRange() const;
    ChartRange getVisibleRange() const;
    
    // ===== 图表配置 =====
    
    // 设置图表配置
    void setChartConfig(const ChartConfig& config);
    ChartConfig getChartConfig() const;
    
    // 单独的配置设置
    void setShowGrid(bool show);
    void setAutoScale(bool enable);
    void setShowLegend(bool show);
    void setShowDataPoints(bool show);
    void setShowTrendLine(bool show);
    
    // 获取配置状态
    bool isGridVisible() const;
    bool isAutoScaleEnabled() const;
    bool isLegendVisible() const;
    
    // ===== 范围设置 =====
    
    // 设置显示范围
    void setXRange(double min, double max);
    void setYRange(double min, double max);
    void setRange(const ChartRange& range);
    
    // 自动调整范围
    void autoFitRange();
    
    // 缩放
    void zoomIn(double factor = 1.2);
    void zoomOut(double factor = 1.2);
    void resetZoom();
    
    // ===== 分析功能 =====
    
    // 执行线性回归
    RegressionResult performLinearRegression();
    
    // 获取当前统计信息
    DataStatistics getCurrentStatistics() const;
    ErrorStatistics getCurrentErrorStatistics() const;
    
    // ===== 3D数据准备 =====
    
    // 准备3D数据（用于外部3D渲染）
    Data3D prepare3DData() const;
    
    // ===== 导出功能 =====
    
    // 导出为图片
    bool exportAsImage(const QString& filename, int width = 800, int height = 600);
    
    // 导出为PDF
    bool exportAsPDF(const QString& filename);
    
    // 导出数据为CSV
    bool exportDataAsCSV(const QString& filename);
    
    // ===== 交互功能 =====
    
    // 设置交互模式
    void setInteractionEnabled(bool enable);
    
    // 设置工具提示格式化函数
    void setTooltipFormatter(std::function<QString(double x, double y)> formatter);
    
signals:
    // 数据点被点击
    void dataPointClicked(double x, double y);
    
    // 范围改变
    void rangeChanged(const ChartRange& range);
    
    // 数据更新
    void dataUpdated(int pointCount);
    
    // 鼠标悬停在数据点上
    void dataPointHovered(double x, double y);
    
public slots:
    // 更新图表显示
    void updateChart();
    
    // 应用固定条件过滤
    void applyFixedConditions(double fixedValue, const QString& fixedParameter);
    
protected:
    // 重写绘制事件（如果需要自定义绘制）
    void resizeEvent(QResizeEvent* event) override;
    
private slots:
    // 处理QCustomPlot的信号
    void onMouseMove(QMouseEvent* event);
    void onMousePress(QMouseEvent* event);
    void onRangeChanged();
    void onSelectionChanged();
    
private:
    // 私有实现类
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // QCustomPlot实例
    QCustomPlot* customPlot;
    
    // 当前图表类型
    ChartType currentChartType;
    
    // 当前数据
    std::vector<MeasurementData> measurementData;
    std::vector<DataPoint> chartData;
    
    // 图表配置
    ChartConfig chartConfig;
    
    // 分析器
    std::unique_ptr<CsvAnalyzer> analyzer;
    
    // ===== 私有方法 =====
    
    // 初始化图表
    void initializeChart();
    
    // 设置图表样式
    void setupChartStyle();
    
    // 更新不同类型的图表
    void updateDistanceCapacitanceChart();
    void updateAngleCapacitanceChart();
    void updateTemperatureCapacitanceChart();
    void updateErrorAnalysisChart();
    void update3DChart();
    
    // 绘制数据
    void plotLineChart(const std::vector<DataPoint>& points);
    void plotScatterChart(const std::vector<DataPoint>& points);
    void plotErrorChart(const std::vector<DataPoint>& points);
    
    // 添加趋势线
    void addTrendLine(const std::vector<DataPoint>& points);
    
    // 添加误差带
    void addErrorBand(const std::vector<DataPoint>& points, 
                     const std::vector<double>& errors);
    
    // 设置轴标签
    void setAxisLabels(const QString& xLabel, const QString& yLabel);
    
    // 格式化工具提示
    QString formatTooltip(double x, double y) const;
    
    // 数据转换
    QVector<double> toQVector(const std::vector<double>& vec) const;
    std::vector<DataPoint> convertToDataPoints(
        const std::vector<MeasurementData>& data) const;
    
    // 查找最近的数据点
    DataPoint findNearestPoint(double x, double y) const;
    
    // 颜色映射（用于温度）
    QColor temperatureToColor(double temperature) const;
};

#endif // CHART_DISPLAY_WIDGET_H