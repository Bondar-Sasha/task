#include "ns3/core-module.h"
#include "env/environment_config.h"
#include "simulation/adhoc_simulator.h"
#include "simulation/group_simulator.h"
#include "writers/csv_writer.h"
#include "analysis/analysis_methods.h"
#include "analysis/queueing_models.h" 

#include <iostream>
#include <vector>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NetworkSimulation");

// Структура для хранения результатов анализа различными методами
struct AnalysisResults {
    std::vector<double> lambdas;
    std::vector<double> actualThroughputs;
    std::vector<double> actualDelays;
    std::vector<double> meanValueAnalysis;
    std::vector<double> globalBalanceMethod;
    std::vector<double> gordonNewellMethod;
    std::vector<double> buzenMethod;
    std::vector<double> meanValueDelay;
};

// Функция для анализа результатов различными методами
AnalysisResults AnalyzeWithAllMethods(const std::vector<SimulationMetrics>& results, 
                      const SimulationConfig& config,
                                      bool isAdHoc,
                                      const std::vector<double>& lambdaValues) {
    AnalysisResults analysisResults;
    AnalysisMethods analyzer;
    
    int numNodes = isAdHoc ? config.numNodes : config.numGroups;
    double serviceRate = config.serviceRate;
    int avgBufferSize = 0;
    
    // Вычисляем средний размер буфера
    if (!config.nodeBuffers.empty()) { // Добавлена проверка на пустой вектор
        for (size_t i = 0; i < config.nodeBuffers.size(); ++i) {
            avgBufferSize += config.nodeBuffers[i];
        }
        avgBufferSize /= config.nodeBuffers.size();
    }
    
    for (size_t i = 0; i < results.size(); ++i) {
        double lambda = lambdaValues[i];  // Используем реальные значения lambda
        double mu = serviceRate;  // Интенсивность обслуживания
        
        // Сохраняем исходные данные
        analysisResults.lambdas.push_back(lambda);
        analysisResults.actualThroughputs.push_back(results[i].throughput);
        analysisResults.actualDelays.push_back(results[i].delay);
        
        // Применяем разные методы анализа
        analysisResults.meanValueAnalysis.push_back(analyzer.MeanValueAnalysis(lambda, mu, avgBufferSize, numNodes));
        analysisResults.globalBalanceMethod.push_back(analyzer.GlobalBalanceMethod(lambda, mu, avgBufferSize, numNodes));
        analysisResults.gordonNewellMethod.push_back(analyzer.GordonNewellMethod(lambda, mu, avgBufferSize, numNodes));
        analysisResults.buzenMethod.push_back(analyzer.BuzenMethod(lambda, mu, avgBufferSize, numNodes));
        
        // Используем правильные методы MVA для расчета задержки в зависимости от режима
        // Режим 1 (Ad-hoc): итерационный MVA для 9 узлов
        // Режим 2 (Group): иерархический MVA с агрегацией (3 подсети по 3 узла)
        if (isAdHoc) {
            // Режим 1: Ad-hoc (9 узлов по отдельности) - итерационный MVA
            analysisResults.meanValueDelay.push_back(analyzer.CalculateMVADelay_Mode1(lambda, mu, avgBufferSize, numNodes));
        } else {
            // Режим 2: Group (3 подсети по 3 узла) - иерархический MVA с агрегацией (FES)
            int nodesPerGroup = (numNodes > 0 && config.numGroups > 0) ? numNodes / config.numGroups : 3;
            analysisResults.meanValueDelay.push_back(analyzer.CalculateMVADelay_Mode2(lambda, mu, avgBufferSize, config.numGroups, nodesPerGroup));
        }
    }
    
    return analysisResults;
}

void PerformComparativeAnalysis(const std::vector<SimulationMetrics>& adHocResults,
                              const std::vector<SimulationMetrics>& groupResults,
                              const SimulationConfig& config) {
    std::cout << "\n=== COMPARATIVE ANALYSIS ===" << std::endl;
    
    AnalysisMethods analyzer;
    
    double adHocAvgThroughput = 0, groupAvgThroughput = 0;
    double adHocAvgDelay = 0, groupAvgDelay = 0;
    double adHocAvgLoss = 0, groupAvgLoss = 0;
    
    for (size_t i = 0; i < adHocResults.size(); ++i) {
        adHocAvgThroughput += adHocResults[i].throughput;
        adHocAvgDelay += adHocResults[i].delay;
        adHocAvgLoss += adHocResults[i].packetLoss;
        
        groupAvgThroughput += groupResults[i].throughput;
        groupAvgDelay += groupResults[i].delay;
        groupAvgLoss += groupResults[i].packetLoss;
    }
    
    adHocAvgThroughput /= adHocResults.size();
    adHocAvgDelay /= adHocResults.size();
    adHocAvgLoss /= adHocResults.size();
    
    groupAvgThroughput /= groupResults.size();
    groupAvgDelay /= groupResults.size();
    groupAvgLoss /= groupResults.size();
    
    std::cout << "Average Performance:" << std::endl;
    std::cout << "  Throughput - AdHoc: " << adHocAvgThroughput << " Mbps, Group: " << groupAvgThroughput << " Mbps" << std::endl;
    std::cout << "  Delay - AdHoc: " << adHocAvgDelay << " s, Group: " << groupAvgDelay << " s" << std::endl;
    std::cout << "  Packet Loss - AdHoc: " << adHocAvgLoss * 100 << "%, Group: " << groupAvgLoss * 100 << "%" << std::endl;
    
    double throughputRatio = (groupAvgThroughput > 0) ? adHocAvgThroughput / groupAvgThroughput : 0;
    double delayRatio = (groupAvgDelay > 0) ? adHocAvgDelay / groupAvgDelay : 0;
    
    std::cout << "\nEfficiency Analysis:" << std::endl;
    std::cout << "  Throughput Ratio (AdHoc/Group): " << throughputRatio << std::endl;
    std::cout << "  Delay Ratio (AdHoc/Group): " << delayRatio << std::endl;
    
    if (throughputRatio > 1.1) {
        std::cout << "  -> AdHoc network shows BETTER throughput" << std::endl;
    } else if (throughputRatio < 0.9) {
        std::cout << "  -> Group network shows BETTER throughput" << std::endl;
    } else {
        std::cout << "  -> Both networks show SIMILAR throughput" << std::endl;
    }
    
    double adHocThroughputStd = 0, groupThroughputStd = 0;
    for (size_t i = 0; i < adHocResults.size(); ++i) {
        adHocThroughputStd += std::pow(adHocResults[i].throughput - adHocAvgThroughput, 2);
        groupThroughputStd += std::pow(groupResults[i].throughput - groupAvgThroughput, 2);
    }
    
    adHocThroughputStd = std::sqrt(adHocThroughputStd / adHocResults.size());
    groupThroughputStd = std::sqrt(groupThroughputStd / groupResults.size());
    
    double adHocCV = (adHocAvgThroughput > 0) ? adHocThroughputStd / adHocAvgThroughput : 0;
    double groupCV = (groupAvgThroughput > 0) ? groupThroughputStd / groupAvgThroughput : 0;
    
    std::cout << "\nStability Analysis (Coefficient of Variation):" << std::endl;
    std::cout << "  AdHoc CV: " << adHocCV * 100 << "%" << std::endl;
    std::cout << "  Group CV: " << groupCV * 100 << "%" << std::endl;
    
    if (adHocCV < groupCV) {
        std::cout << "  -> AdHoc network is MORE stable" << std::endl;
    } else {
        std::cout << "  -> Group network is MORE stable" << std::endl;
    }
    
    std::cout << "\nComparison with Analytical Models:" << std::endl;
    
    double avgLoad = (adHocAvgThroughput + groupAvgThroughput) / (2.0 * config.dataRateMbps);
    
    // Используем MM1KAnalysis с большим bufferSize для симуляции M/M/1
    double mm1Prediction = analyzer.MM1KAnalysis(avgLoad * config.dataRateMbps, config.serviceRate, 1000);
    double adHocModelPrediction = analyzer.AdHocThroughputModel(avgLoad, config.dataRateMbps, config.numNodes);
    double groupModelPrediction = analyzer.GroupThroughputModel(avgLoad, config.dataRateMbps, config.numGroups);
    
    std::cout << "  M/M/1 Model Prediction: " << mm1Prediction << " Mbps" << std::endl;
    std::cout << "  AdHoc Model Prediction: " << adHocModelPrediction << " Mbps" << std::endl;
    std::cout << "  Group Model Prediction: " << groupModelPrediction << " Mbps" << std::endl;
    std::cout << "  Actual AdHoc: " << adHocAvgThroughput << " Mbps" << std::endl;
    std::cout << "  Actual Group: " << groupAvgThroughput << " Mbps" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "Starting NS-3 Network Analysis with ENV Configuration..." << std::endl;
    std::cout << "==========================================================" << std::endl;
    
    SimulationConfig config = EnvironmentConfig::Load("scratch/.env");
    
    std::vector<SimulationMetrics> adHocResults;
    std::vector<SimulationMetrics> groupResults;
    std::vector<double> lambdaValues;  // Сохраняем значения lambda
    
    AnalysisMethods analyzer;
    
    int result = system("mkdir -p scratch/public");
    (void)result;
    
    int numPoints = static_cast<int>((config.lambdaEnd - config.lambdaStart) / config.lambdaStep) + 1;
    std::cout << "Running " << numPoints << " simulation points..." << std::endl;
    std::cout << "Lambda range: " << config.lambdaStart << " to " << config.lambdaEnd 
              << " step " << config.lambdaStep << std::endl;
    std::cout << "Nodes: " << config.numNodes << ", Groups: " << config.numGroups << std::endl;
    std::cout << "==========================================================" << std::endl;
    
    for (int i = 0; i < numPoints; ++i) {
        double lambda = config.lambdaStart + i * config.lambdaStep;
        lambdaValues.push_back(lambda);  // Сохраняем lambda
        std::cout << "\n=== Simulation Point " << (i + 1) << "/" << numPoints 
                  << " (Lambda=" << lambda << ") ===" << std::endl;
        
        SimulationMetrics adHocMetrics = AdHocSimulator::Run(config, lambda);
        adHocResults.push_back(adHocMetrics);
        
        std::cout << std::endl;
        
        SimulationMetrics groupMetrics = GroupSimulator::Run(config, lambda);
        groupResults.push_back(groupMetrics);
        
        std::cout << "----------------------------------------" << std::endl;
        
        double adHocModel = analyzer.AdHocThroughputModel(adHocMetrics.load, config.dataRateMbps, config.numNodes);
        double groupModel = analyzer.GroupThroughputModel(groupMetrics.load, config.dataRateMbps, config.numGroups);
        
        std::cout << "Point Analysis:" << std::endl;
        std::cout << "  AdHoc - Actual: " << adHocMetrics.throughput << " Mbps, Model: " << adHocModel << " Mbps" << std::endl;
        std::cout << "  Group - Actual: " << groupMetrics.throughput << " Mbps, Model: " << groupModel << " Mbps" << std::endl;
    }
    
    std::cout << "\n==========================================================" << std::endl;
    std::cout << "=== FINAL ANALYSIS RESULTS ===" << std::endl;
    std::cout << "==========================================================" << std::endl;
    
    PerformComparativeAnalysis(adHocResults, groupResults, config);
    
    std::cout << "\n=== DETAILED METHODOLOGY ANALYSIS ===" << std::endl;
    
    std::vector<double> adHocThroughputs, groupThroughputs;
    std::vector<double> adHocLoads, groupLoads;
    
    for (const auto& metrics : adHocResults) {
        adHocThroughputs.push_back(metrics.throughput);
        adHocLoads.push_back(metrics.load);
    }
    
    for (const auto& metrics : groupResults) {
        groupThroughputs.push_back(metrics.throughput);
        groupLoads.push_back(metrics.load);
    }
    
    double adHocMeanThroughput = analyzer.GlobalMeanAnalysis(adHocThroughputs);
    double groupMeanThroughput = analyzer.GlobalMeanAnalysis(groupThroughputs);
    double adHocMeanLoad = analyzer.GlobalMeanAnalysis(adHocLoads);
    double groupMeanLoad = analyzer.GlobalMeanAnalysis(groupLoads);
    
    std::cout << "Global Mean Analysis:" << std::endl;
    std::cout << "  Ad-Hoc - Mean Throughput: " << adHocMeanThroughput << " Mbps, Mean Load: " << adHocMeanLoad << std::endl;
    std::cout << "  Group  - Mean Throughput: " << groupMeanThroughput << " Mbps, Mean Load: " << groupMeanLoad << std::endl;
    
    // Используем MM1KAnalysis с большим bufferSize для симуляции M/M/1
    double adHocMM1 = analyzer.MM1KAnalysis(adHocMeanLoad * config.dataRateMbps, config.serviceRate, 1000);
    double groupMM1 = analyzer.MM1KAnalysis(groupMeanLoad * config.dataRateMbps, config.serviceRate, 1000);
    
    std::cout << "\nM/M/1 Queue Analysis:" << std::endl;
    std::cout << "  Ad-Hoc M/M/1 Throughput: " << adHocMM1 << " Mbps" << std::endl;
    std::cout << "  Group M/M/1 Throughput: " << groupMM1 << " Mbps" << std::endl;
    
    double adHocMM1K = analyzer.MM1KAnalysis(adHocMeanLoad * config.dataRateMbps, config.serviceRate, config.bufferSize);
    double groupMM1K = analyzer.MM1KAnalysis(groupMeanLoad * config.dataRateMbps, config.serviceRate, config.bufferSize);
    
    std::cout << "\nM/M/1/K Queue Analysis (with buffer " << config.bufferSize << "):" << std::endl;
    std::cout << "  Ad-Hoc M/M/1/K Throughput: " << adHocMM1K << " Mbps" << std::endl;
    std::cout << "  Group M/M/1/K Throughput: " << groupMM1K << " Mbps" << std::endl;
    
    double adHocModel = analyzer.AdHocThroughputModel(adHocMeanLoad, config.dataRateMbps, config.numNodes);
    double groupModel = analyzer.GroupThroughputModel(groupMeanLoad, config.dataRateMbps, config.numGroups);
    
    std::cout << "\nSpecialized Network Models:" << std::endl;
    std::cout << "  Ad-Hoc Model Throughput: " << adHocModel << " Mbps" << std::endl;
    std::cout << "  Group Model Throughput: " << groupModel << " Mbps" << std::endl;
    
    AnalysisResults adHocAnalysis = AnalyzeWithAllMethods(adHocResults, config, true, lambdaValues);
    AnalysisResults groupAnalysis = AnalyzeWithAllMethods(groupResults, config, false, lambdaValues);
    
    CsvWriter::WriteResults(adHocResults, groupResults, config, "scratch/public/simulation_results.csv");
    CsvWriter::WriteAnalysis(adHocResults, groupResults, config, "scratch/public/analysis_results.csv");
    CsvWriter::WriteNodeStatistics(adHocResults, groupResults, config, "scratch/public/node_statistics.csv");
    
    // Записываем результаты анализа для построения графиков с четырьмя методами
    CsvWriter::WriteAnalysisWithMethods(adHocAnalysis, groupAnalysis, "scratch/public/analysis_with_methods.csv");
    
    return 0;
}