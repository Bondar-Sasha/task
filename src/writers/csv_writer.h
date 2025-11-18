#pragma once

#include "../env/environment_config.h"
#include "../metrics/simulation_metrics.h"
#include <vector>
#include <string>

// Структура для хранения результатов анализа различными методами
struct AnalysisResults;

class CsvWriter {
public:
    static void WriteResults(const std::vector<SimulationMetrics>& adHocResults,
                            const std::vector<SimulationMetrics>& groupResults,
                            const SimulationConfig& config,
                            const std::string& filename);
    
    static void WriteAnalysis(const std::vector<SimulationMetrics>& adHocResults,
                             const std::vector<SimulationMetrics>& groupResults,
                             const SimulationConfig& config,
                             const std::string& filename);
    
    static void WriteNodeStatistics(const std::vector<SimulationMetrics>& adHocResults,
                                   const std::vector<SimulationMetrics>& groupResults,
                                   const SimulationConfig& config,
                                   const std::string& filename);
                                   
    // Новый метод для записи результатов анализа четырьмя методами
    static void WriteAnalysisWithMethods(const AnalysisResults& adHocAnalysis,
                                        const AnalysisResults& groupAnalysis,
                                        const std::string& filename);
};

