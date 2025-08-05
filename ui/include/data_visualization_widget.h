// ui/include/data_visualization_widget.h
#ifndef DATA_VISUALIZATION_WIDGET_H
#define DATA_VISUALIZATION_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QDialog>
#include <memory>
#include <vector>
#include "chart_display_widget.h"
#include "../../src/models/include/measurement_data.h"
#include "../../src/data/include/csv_analyzer.h"

// 前向声明
namespace Ui {
    class MainWindow;
}

class DataVisualizationWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit DataVisualizationWidget(QWidget *parent = nullptr);
    ~DataVisualizationWidget();
    
    // 设置UI引用（从MainWindow传入）
    void setUiReferences(Ui::MainWindow* ui);
    
    // ===== 数据管理 =====
    
    // 加载CSV文件
    bool loadCsvFile(const std::string& filename);
    
    // 设置数据（从内存）
    void setData(const std::vector<MeasurementData>& data);
    
    // 添加实时数据
    void addRealTimeData(const MeasurementData& data);
    
    // 清空数据
    void clearData();
    
    // 获取数据信息
    bool hasData() const;
    int getDataPointCount() const;
    
    // ===== 图表控制 =====
    
    // 设置图表类型
    void setChartType(ChartType type);
    ChartType getCurrentChartType() const;
    
    // 设置数据源
    void setDataSource(DataSource source);
    DataSource getDataSource() const;
    
    // ===== 统计信息 =====
    
    // 获取当前统计信息
    DataStatistics getStatistics() const;
    ErrorStatistics getErrorStatistics() const;
    
    // ===== 导出功能 =====
    
    // 导出图表为图片
    bool exportChartAsImage(const std::string& filename);
    
    // 导出分析结果
    bool exportAnalysisResults(const std::string& filename);
    
signals:
    // 数据加载完成
    void dataLoaded(int recordCount);
    
    // 图表类型改变
    void chartTypeChanged(ChartType type);
    
    // 统计信息更新
    void statisticsUpdated(const DataStatistics& stats);
    
    // 错误发生
    void errorOccurred(const QString& error);
    
public slots:
    // UI事件处理
    void onChartTypeChanged(int index);
    void onDataSourceChanged(int index);
    void onLoadCSVClicked();
    void onChartRangeChanged();
    void onShowGridChanged(bool checked);
    void onAutoScaleChanged(bool checked);
    void onExportChartClicked();
    
    // 数据更新
    void updateDisplay();
    void updateStatistics();
    
private:
    // UI指针
    Ui::MainWindow* ui;
    
    // 图表显示控件
    ChartDisplayWidget* chartWidget;
    
    // 数据分析器
    std::unique_ptr<CsvAnalyzer> analyzer;
    
    // 当前数据
    std::vector<MeasurementData> currentData;
    std::vector<MeasurementData> filteredData;
    
    // 当前设置
    ChartType currentChartType;
    DataSource currentDataSource;
    
    // 固定条件值（用于2D图表）
    struct FixedConditions {
        double fixedHeight = 50.0;
        double fixedAngle = 0.0;
        double fixedTemperature = 25.0;
        
        double heightTolerance = 2.0;
        double angleTolerance = 2.0;
        double tempTolerance = 1.0;
    } fixedConditions;
    
    // 统计信息缓存
    mutable DataStatistics cachedStatistics;
    mutable ErrorStatistics cachedErrorStatistics;
    mutable bool statisticsValid = false;
    
    // ===== 私有方法 =====
    
    // 初始化
    void initialize();
    void setupUI();
    void connectSignals();
    
    // 更新UI控件
    void updateChartTypeCombo();
    void updateDataSourceCombo();
    void updateRangeControls();
    void updateStatisticsDisplay();
    void updateChartInfo();
    
    // 数据处理
    void processLoadedData();
    void applyFilters();
    void prepareChartData();
    
    // 根据图表类型准备数据
    std::vector<DataPoint> prepareDataForCurrentChart();
    std::vector<DataPoint> prepareDistanceCapacitanceData();
    std::vector<DataPoint> prepareAngleCapacitanceData();
    std::vector<DataPoint> prepareTemperatureCapacitanceData();
    std::vector<DataPoint> prepareErrorAnalysisData();
    Data3D prepare3DData();
    
    // 固定条件对话框
    void showFixedConditionsDialog();
    
    // 获取合适的固定值
    std::vector<double> getAvailableHeights();
    std::vector<double> getAvailableAngles();
    std::vector<double> getAvailableTemperatures();
    
    // 验证数据
    bool validateData();
    
    // 错误处理
    void showError(const QString& message);
    void showWarning(const QString& message);
    void showInfo(const QString& message);
    
    // 格式化显示
    QString formatStatValue(double value, int precision = 2) const;
    QString formatErrorValue(double value, int precision = 3) const;
    
    // 计算辅助
    void calculateStatistics() const;
    void calculateErrorStatistics() const;
};

// ===== 固定条件选择对话框 =====

class FixedConditionsDialog : public QDialog {
    Q_OBJECT
    
public:
    struct Conditions {
        double height = 50.0;
        double angle = 0.0;
        double temperature = 25.0;
        double heightTolerance = 2.0;
        double angleTolerance = 2.0;
        double tempTolerance = 1.0;
    };
    
    explicit FixedConditionsDialog(QWidget *parent = nullptr);
    
    // 设置可用值
    void setAvailableHeights(const std::vector<double>& heights);
    void setAvailableAngles(const std::vector<double>& angles);
    void setAvailableTemperatures(const std::vector<double>& temps);
    
    // 设置当前条件
    void setConditions(const Conditions& conditions);
    
    // 获取选择的条件
    Conditions getConditions() const;
    
private:
    QComboBox* heightCombo;
    QComboBox* angleCombo;
    QComboBox* tempCombo;
    
    QDoubleSpinBox* heightToleranceSpin;
    QDoubleSpinBox* angleToleranceSpin;
    QDoubleSpinBox* tempToleranceSpin;
    
    void setupUI();
};

#endif // DATA_VISUALIZATION_WIDGET_H