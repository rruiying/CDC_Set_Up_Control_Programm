#include "data/include/data_processor.h"
#include "utils/include/math_utils.h"
#include <algorithm>
#include <numeric>

DataProcessor::DataProcessor(QObject* parent)
    : QObject(parent)
{
    m_config.filterType = FilterType::MovingAverage;
    m_config.windowSize = 10;
    m_config.smoothingFactor = 0.3f;
    m_config.enableOutlierRemoval = true;
    m_config.outlierThreshold = 3.0f; // 3倍标准差
}

DataProcessor::~DataProcessor() {
}

SensorData DataProcessor::processData(const SensorData& rawData) {
    SensorData processed = rawData;
    
    // 处理温度
    if (rawData.isValid.temperature) {
        m_temperatureHistory.enqueue(rawData.temperature);
        if (m_temperatureHistory.size() > m_config.windowSize) {
            m_temperatureHistory.dequeue();
        }
        
        if (m_config.enableOutlierRemoval && 
            isOutlier(rawData.temperature, 
                     QList<float>(m_temperatureHistory.begin(), 
                                 m_temperatureHistory.end()))) {
            emit outlierDetected("Temperature", rawData.temperature);
        } else {
            processed.temperature = applyFilter(rawData.temperature, 
                                              m_temperatureHistory);
        }
    }
    
    // 处理高度（使用平均值）
    if (rawData.isValid.distanceUpper1 && rawData.isValid.distanceUpper2) {
        float avgHeight = (rawData.distanceUpper1 + rawData.distanceUpper2) / 2.0f;
        m_heightHistory.enqueue(avgHeight);
        if (m_heightHistory.size() > m_config.windowSize) {
            m_heightHistory.dequeue();
        }
        processed.calculatedHeight = applyFilter(avgHeight, m_heightHistory);
    }
    
    // 类似处理其他参数...
    
    emit dataProcessed(processed);
    return processed;
}

QList<SensorData> DataProcessor::processBatch(const QList<SensorData>& rawDataList) {
    QList<SensorData> processedList;
    for (const auto& data : rawDataList) {
        processedList.append(processData(data));
    }
    return processedList;
}

void DataProcessor::setConfig(const ProcessingConfig& config) {
    m_config = config;
    
    // 调整历史缓存大小
    while (m_temperatureHistory.size() > config.windowSize) {
        m_temperatureHistory.dequeue();
    }
    // 其他历史缓存类似...
}

void DataProcessor::setFilterType(FilterType type) {
    m_config.filterType = type;
}

void DataProcessor::setWindowSize(int size) {
    m_config.windowSize = size;
}

void DataProcessor::setSmoothingFactor(float factor) {
    m_config.smoothingFactor = MathUtils::clamp(factor, 0.0f, 1.0f);
}

float DataProcessor::calculateAverage(const QList<float>& values) {
    if (values.isEmpty()) return 0.0f;
    return std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
}

float DataProcessor::calculateStdDev(const QList<float>& values) {
    if (values.size() <= 1) return 0.0f;
    
    float mean = calculateAverage(values);
    float sum = 0.0f;
    
    for (float value : values) {
        float diff = value - mean;
        sum += diff * diff;
    }
    
    return std::sqrt(sum / (values.size() - 1));
}

QPair<float, float> DataProcessor::calculateMinMax(const QList<float>& values) {
    if (values.isEmpty()) {
        return qMakePair(0.0f, 0.0f);
    }
    
    auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
    return qMakePair(*minIt, *maxIt);
}

bool DataProcessor::isOutlier(float value, const QList<float>& history) {
    if (history.size() < 3) return false;
    
    float mean = calculateAverage(history);
    float stdDev = calculateStdDev(history);
    
    return std::abs(value - mean) > m_config.outlierThreshold * stdDev;
}

float DataProcessor::applyFilter(float value, QQueue<float>& history) {
    switch (m_config.filterType) {
        case FilterType::None:
            return value;
            
        case FilterType::MovingAverage:
            return movingAverage(history);
            
        case FilterType::ExponentialSmoothing:
            if (!history.isEmpty()) {
                return exponentialSmooth(value, history.last());
            }
            return value;
            
        case FilterType::MedianFilter:
            return medianFilter(history);
            
        case FilterType::KalmanFilter:
            // TODO: 实现卡尔曼滤波
            return value;
            
        default:
            return value;
    }
}

float DataProcessor::movingAverage(const QQueue<float>& data) {
    if (data.isEmpty()) return 0.0f;
    
    float sum = std::accumulate(data.begin(), data.end(), 0.0f);
    return sum / data.size();
}

float DataProcessor::exponentialSmooth(float current, float previous) {
    return m_config.smoothingFactor * current + 
           (1.0f - m_config.smoothingFactor) * previous;
}

float DataProcessor::medianFilter(QQueue<float> data) {
    if (data.isEmpty()) return 0.0f;
    
    std::vector<float> sorted(data.begin(), data.end());
    std::sort(sorted.begin(), sorted.end());
    
    size_t n = sorted.size();
    if (n % 2 == 0) {
        return (sorted[n/2 - 1] + sorted[n/2]) / 2.0f;
    } else {
        return sorted[n/2];
    }
}