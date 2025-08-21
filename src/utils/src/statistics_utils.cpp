#include "../include/statistics_utils.h"
#include <cmath>
#include <algorithm>
#include <numeric>

double StatisticsUtils::mean(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

double StatisticsUtils::variance(const std::vector<double>& data) {
    if (data.size() < 2) return 0.0;
    double m = mean(data);
    double sum = 0.0;
    for (double val : data) {
        double diff = val - m;
        sum += diff * diff;
    }
    return sum / (data.size() - 1);
}

double StatisticsUtils::stdDev(const std::vector<double>& data) {
    return std::sqrt(variance(data));
}

double StatisticsUtils::median(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    std::vector<double> sorted = data;
    std::sort(sorted.begin(), sorted.end());
    size_t n = sorted.size();
    if (n % 2 == 0) {
        return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
    }
    return sorted[n/2];
}

double StatisticsUtils::variance(const std::vector<double>& data, double mean) {
    if (data.size() < 2) return 0.0;
    
    double sum = 0.0;
    for (double val : data) {
        double diff = val - mean;
        sum += diff * diff;
    }
    return sum / (data.size() - 1);
}

double StatisticsUtils::stdDev(const std::vector<double>& data, double mean) {
    return std::sqrt(variance(data, mean));
}

double StatisticsUtils::skewness(const std::vector<double>& data, double mean, double stdDev) {
    if (data.size() < 3 || stdDev < 1e-10) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double val : data) {
        sum += std::pow((val - mean) / stdDev, 3);
    }
    
    size_t n = data.size();
    return sum * n / ((n - 1) * (n - 2));
}

double StatisticsUtils::kurtosis(const std::vector<double>& data, double mean, double stdDev) {
    if (data.size() < 4 || stdDev < 1e-10) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double val : data) {
        sum += std::pow((val - mean) / stdDev, 4);
    }
    
    size_t n = data.size();
    double numerator = n * (n + 1) * sum;
    double denominator = (n - 1) * (n - 2) * (n - 3);
    double adjustment = 3.0 * (n - 1) * (n - 1) / ((n - 2) * (n - 3));
    
    return numerator / denominator - adjustment;
}

StatisticsUtils::LinearRegressionResult 
StatisticsUtils::linearRegression(const std::vector<double>& x, 
                                  const std::vector<double>& y) {
    LinearRegressionResult result{};
    
    if (x.size() != y.size() || x.size() < 2) {
        return result;
    }
    
    double n = static_cast<double>(x.size());
    double sumX = std::accumulate(x.begin(), x.end(), 0.0);
    double sumY = std::accumulate(y.begin(), y.end(), 0.0);
    double sumXY = 0.0, sumX2 = 0.0;
    
    for (size_t i = 0; i < x.size(); ++i) {
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
    }
    
    double denominator = n * sumX2 - sumX * sumX;
    if (std::abs(denominator) < 1e-10) {
        return result;
    }
    
    result.slope = (n * sumXY - sumX * sumY) / denominator;
    result.intercept = (sumY - result.slope * sumX) / n;
    
    // 计算R²和RMSE
    double ssTotal = 0.0, ssResidual = 0.0;
    double meanY = sumY / n;
    result.residuals.resize(x.size());
    
    for (size_t i = 0; i < x.size(); ++i) {
        double predicted = result.slope * x[i] + result.intercept;
        double residual = y[i] - predicted;
        result.residuals[i] = residual;
        ssResidual += residual * residual;
        ssTotal += (y[i] - meanY) * (y[i] - meanY);
    }
    
    result.r2 = (ssTotal > 0) ? 1.0 - (ssResidual / ssTotal) : 0.0;
    result.rmse = std::sqrt(ssResidual / n);
    
    return result;
}

StatisticsUtils::ErrorMetrics 
StatisticsUtils::calculateError(const std::vector<double>& actual,
                               const std::vector<double>& predicted) {
    ErrorMetrics metrics{};
    
    if (actual.size() != predicted.size() || actual.empty()) {
        return metrics;
    }
    
    double n = static_cast<double>(actual.size());
    double sumAE = 0.0, sumSE = 0.0, sumAPE = 0.0;
    int validAPE = 0;
    
    for (size_t i = 0; i < actual.size(); ++i) {
        double error = actual[i] - predicted[i];
        sumAE += std::abs(error);
        sumSE += error * error;
        
        if (std::abs(actual[i]) > 1e-10) {
            sumAPE += std::abs(error / actual[i]);
            validAPE++;
        }
    }
    
    metrics.mae = sumAE / n;
    metrics.mse = sumSE / n;
    metrics.rmse = std::sqrt(metrics.mse);
    metrics.mape = (validAPE > 0) ? (sumAPE / validAPE * 100.0) : 0.0;
    
    return metrics;
}