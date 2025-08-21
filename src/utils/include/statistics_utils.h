#ifndef STATISTICS_UTILS_H
#define STATISTICS_UTILS_H

#include <vector>

class StatisticsUtils {
public:

    static double mean(const std::vector<double>& data);
    static double stdDev(const std::vector<double>& data);
    static double stdDev(const std::vector<double>& data, double mean);
    static double variance(const std::vector<double>& data);
    static double variance(const std::vector<double>& data, double mean);
    static double median(const std::vector<double>& data);

    static double skewness(const std::vector<double>& data, double mean, double stdDev);
    static double kurtosis(const std::vector<double>& data, double mean, double stdDev);
    
    // 回归分析
    struct LinearRegressionResult {
        double slope;
        double intercept;
        double r2;
        double rmse;
        std::vector<double> residuals;
    };
    static LinearRegressionResult linearRegression(
        const std::vector<double>& x,
        const std::vector<double>& y);
    
    // 误差分析
    struct ErrorMetrics {
        double mae;    // Mean Absolute Error
        double mse;    // Mean Squared Error  
        double rmse;   // Root Mean Squared Error
        double mape;   // Mean Absolute Percentage Error
    };
    static ErrorMetrics calculateError(
        const std::vector<double>& actual,
        const std::vector<double>& predicted);
};

#endif