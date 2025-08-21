#ifndef CSV_ANALYZER_H
#define CSV_ANALYZER_H

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include "../../models/include/measurement_data.h"
#include "../../models/include/data_statistics.h"

class CsvAnalyzer {
public:
    CsvAnalyzer();
    ~CsvAnalyzer();
    
    bool loadCsvFile(const std::string& filename);
    
    void setData(const std::vector<MeasurementData>& data);
    
    // 清空数据
    void clearData();
    
    std::vector<MeasurementData> getData() const;
    
    // 获取数据数量
    size_t getDataCount() const;

    bool saveCsvFile(const std::string& filename, 
                     const std::vector<MeasurementData>& data);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // CSV_ANALYZER_H