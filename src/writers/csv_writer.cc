#include "csv_writer.h"
#include "../analysis/analysis_methods.h"
#include <fstream>
#include <iostream>

// Определение структуры AnalysisResults из main.cc
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

void CsvWriter::WriteResults(const std::vector<SimulationMetrics>& adHocResults,
                             const std::vector<SimulationMetrics>& groupResults,
                             const SimulationConfig& config,
                             const std::string& filename) {
    std::ofstream file(filename);
    
    file << "NetworkType,Lambda,Throughput_Mbps,Delay_s,PacketLoss,Load,"
         << "TxPackets,RxPackets,LostPackets,DroppedPackets,"
         << "TxBytes,RxBytes,Jitter_s,AvgHopCount,"
         << "DeliveryRatio,Goodput_Mbps,"
         << "Node0_Load,Node1_Load,Node2_Load,Node3_Load,Node4_Load,"
         << "Node5_Load,Node6_Load,Node7_Load,Node8_Load,"
         << "SimulationDuration,BufferSize,PacketSize,DataRate_Mbps,LinkDelay_ms,"
         << "MaxPackets,NumNodes,NumGroups,ServiceRate,BuzenMultiplier\n";
    
    double lambda = config.lambdaStart;
    
    for (size_t i = 0; i < adHocResults.size(); ++i) {
        const auto& m = adHocResults[i];
        double deliveryRatio = (m.txPackets > 0) ? (double)m.rxPackets / m.txPackets : 0;
        double goodput = m.rxBytes * 8.0 / (m.simulationTime * 1000000.0);
        
        file << "AdHoc," << lambda << ","
             << m.throughput << "," << m.delay << "," << m.packetLoss << "," << m.load << ","
             << m.txPackets << "," << m.rxPackets << "," << m.lostPackets << "," << m.droppedPackets << ","
             << m.txBytes << "," << m.rxBytes << "," << m.jitter << "," << m.avgHopCount << ","
             << deliveryRatio << "," << goodput << ",";
        
        for (size_t j = 0; j < config.nodeLoads.size(); ++j) {
            file << config.nodeLoads[j];
            if (j < config.nodeLoads.size() - 1) file << ",";
        }
        
        file << "," << config.simulationDuration << "," << config.bufferSize << ","
             << config.packetSize << "," << config.dataRateMbps << "," << config.linkDelayMs << ","
             << config.maxPackets << "," << config.numNodes << "," << config.numGroups << ","
             << config.serviceRate << "," << config.buzenCustomersMultiplier << "\n";
        
        lambda += config.lambdaStep;
    }
    
    lambda = config.lambdaStart;
    for (size_t i = 0; i < groupResults.size(); ++i) {
        const auto& m = groupResults[i];
        double deliveryRatio = (m.txPackets > 0) ? (double)m.rxPackets / m.txPackets : 0;
        double goodput = m.rxBytes * 8.0 / (m.simulationTime * 1000000.0);
        
        file << "Group," << lambda << ","
             << m.throughput << "," << m.delay << "," << m.packetLoss << "," << m.load << ","
             << m.txPackets << "," << m.rxPackets << "," << m.lostPackets << "," << m.droppedPackets << ","
             << m.txBytes << "," << m.rxBytes << "," << m.jitter << "," << m.avgHopCount << ","
             << deliveryRatio << "," << goodput << ",";
        
        for (size_t j = 0; j < config.nodeLoads.size(); ++j) {
            file << config.nodeLoads[j];
            if (j < config.nodeLoads.size() - 1) file << ",";
        }
        
        file << "," << config.simulationDuration << "," << config.bufferSize << ","
             << config.packetSize << "," << config.dataRateMbps << "," << config.linkDelayMs << ","
             << config.maxPackets << "," << config.numNodes << "," << config.numGroups << ","
             << config.serviceRate << "," << config.buzenCustomersMultiplier << "\n";
        
        lambda += config.lambdaStep;
    }
    
    file.close();
    std::cout << "CSV results written to: " << filename << std::endl;
}

void CsvWriter::WriteAnalysis(const std::vector<SimulationMetrics>& adHocResults,
                              const std::vector<SimulationMetrics>& groupResults,
                              const SimulationConfig& config,
                              const std::string& filename) {
    std::ofstream file(filename);
    
    file << "AnalysisType,AdHoc_Value,Group_Value,Parameters\n";
    
    AnalysisMethods analyzer;
    
    std::vector<double> adHocThroughputs, groupThroughputs;
    std::vector<double> adHocLoads, groupLoads;
    std::vector<double> adHocDelays, groupDelays;
    
    for (const auto& metrics : adHocResults) {
        adHocThroughputs.push_back(metrics.throughput);
        adHocLoads.push_back(metrics.load);
        adHocDelays.push_back(metrics.delay);
    }
    
    for (const auto& metrics : groupResults) {
        groupThroughputs.push_back(metrics.throughput);
        groupLoads.push_back(metrics.load);
        groupDelays.push_back(metrics.delay);
    }
    
    double adHocMeanThroughput = analyzer.GlobalMeanAnalysis(adHocThroughputs);
    double groupMeanThroughput = analyzer.GlobalMeanAnalysis(groupThroughputs);
    double adHocMeanLoad = analyzer.GlobalMeanAnalysis(adHocLoads);
    double groupMeanLoad = analyzer.GlobalMeanAnalysis(groupLoads);
    double adHocMeanDelay = analyzer.GlobalMeanAnalysis(adHocDelays);
    double groupMeanDelay = analyzer.GlobalMeanAnalysis(groupDelays);
    
    file << "GlobalMean_Throughput," << adHocMeanThroughput << "," << groupMeanThroughput << ",Mbps\n";
    file << "GlobalMean_Load," << adHocMeanLoad << "," << groupMeanLoad << ",normalized\n";
    file << "GlobalMean_Delay," << adHocMeanDelay << "," << groupMeanDelay << ",seconds\n";
    
    double adHocMM1 = analyzer.MM1KAnalysis(adHocMeanLoad, config.serviceRate, 1000);
    double groupMM1 = analyzer.MM1KAnalysis(groupMeanLoad, config.serviceRate, 1000);
    file << "MM1_Throughput," << adHocMM1 << "," << groupMM1 << ",Mbps\n";
    
    double adHocMM1K = analyzer.MM1KAnalysis(adHocMeanLoad, config.serviceRate, config.bufferSize);
    double groupMM1K = analyzer.MM1KAnalysis(groupMeanLoad, config.serviceRate, config.bufferSize);
    file << "MM1K_Throughput," << adHocMM1K << "," << groupMM1K << ",Mbps\n";
    
    double adHocModel = analyzer.AdHocThroughputModel(adHocMeanLoad, config.dataRateMbps, config.numNodes);
    double groupModel = analyzer.GroupThroughputModel(groupMeanLoad, config.dataRateMbps, config.numGroups);
    file << "Specialized_Throughput," << adHocModel << "," << groupModel << ",Mbps\n";
    
    double adHocDelayModel = analyzer.MMDelayModel(adHocMeanLoad, config.serviceRate);
    double groupDelayModel = analyzer.MMDelayModel(groupMeanLoad, config.serviceRate);
    file << "MM_Delay_Model," << adHocDelayModel << "," << groupDelayModel << ",seconds\n";
    
    double adHocLossModel = analyzer.MMLossModel(adHocMeanLoad, config.serviceRate, config.bufferSize);
    double groupLossModel = analyzer.MMLossModel(groupMeanLoad, config.serviceRate, config.bufferSize);
    file << "MM_Loss_Model," << adHocLossModel << "," << groupLossModel << ",ratio\n";
    
    file << "Simulation_Points," << adHocResults.size() << "," << groupResults.size() << ",count\n";
    file << "Lambda_Range," << config.lambdaStart << "-" << config.lambdaEnd << "," << config.lambdaStart << "-" << config.lambdaEnd << ",packets/sec\n";
    file << "Node_Load_Mode," << config.nodeLoadMode << "," << config.nodeLoadMode << ",type\n";
    
    file.close();
    std::cout << "Analysis CSV written to: " << filename << std::endl;
}

void CsvWriter::WriteNodeStatistics(const std::vector<SimulationMetrics>& adHocResults,
                                   const std::vector<SimulationMetrics>& groupResults,
                                   const SimulationConfig& config,
                                   const std::string& filename) {
    std::ofstream file(filename);
    
    file << "NetworkType,Lambda,NodeID,Throughput_Mbps,Delay_s,"
         << "TxPackets,RxPackets,LostPackets,DeliveryRatio,"
         << "Configured_Load,Effective_Load\n";
    
    double lambda = config.lambdaStart;
    
    for (size_t i = 0; i < adHocResults.size(); ++i) {
        for (uint32_t nodeId = 0; nodeId < config.numNodes; ++nodeId) {
            double nodeThroughput = 0;
            double nodeDelay = 0;
            uint32_t nodeTx = 0;
            uint32_t nodeRx = 0;
            uint32_t nodeLost = 0;
            
            auto throughputIt = adHocResults[i].nodeThroughput.find(nodeId);
            auto delayIt = adHocResults[i].nodeDelay.find(nodeId);
            auto txIt = adHocResults[i].nodeTxPackets.find(nodeId);
            auto rxIt = adHocResults[i].nodeRxPackets.find(nodeId);
            auto lostIt = adHocResults[i].nodeLostPackets.find(nodeId);
            
            if (throughputIt != adHocResults[i].nodeThroughput.end()) {
                nodeThroughput = throughputIt->second;
            }
            if (delayIt != adHocResults[i].nodeDelay.end()) {
                nodeDelay = delayIt->second;
            }
            if (txIt != adHocResults[i].nodeTxPackets.end()) {
                nodeTx = txIt->second;
            }
            if (rxIt != adHocResults[i].nodeRxPackets.end()) {
                nodeRx = rxIt->second;
            }
            if (lostIt != adHocResults[i].nodeLostPackets.end()) {
                nodeLost = lostIt->second;
            }
            
            double deliveryRatio = (nodeTx > 0) ? (double)nodeRx / nodeTx : 0;
            double effectiveLoad = lambda * config.nodeLoads[nodeId];
            
            file << "AdHoc," << lambda << "," << nodeId << ","
                 << nodeThroughput << "," << nodeDelay << ","
                 << nodeTx << "," << nodeRx << "," << nodeLost << ","
                 << deliveryRatio << ","
                 << config.nodeLoads[nodeId] << "," << effectiveLoad << "\n";
        }
        lambda += config.lambdaStep;
    }
    
    lambda = config.lambdaStart;
    for (size_t i = 0; i < groupResults.size(); ++i) {
        for (uint32_t nodeId = 0; nodeId < config.numNodes; ++nodeId) {
            double nodeThroughput = 0;
            double nodeDelay = 0;
            uint32_t nodeTx = 0;
            uint32_t nodeRx = 0;
            uint32_t nodeLost = 0;
            
            auto throughputIt = groupResults[i].nodeThroughput.find(nodeId);
            auto delayIt = groupResults[i].nodeDelay.find(nodeId);
            auto txIt = groupResults[i].nodeTxPackets.find(nodeId);
            auto rxIt = groupResults[i].nodeRxPackets.find(nodeId);
            auto lostIt = groupResults[i].nodeLostPackets.find(nodeId);
            
            if (throughputIt != groupResults[i].nodeThroughput.end()) {
                nodeThroughput = throughputIt->second;
            }
            if (delayIt != groupResults[i].nodeDelay.end()) {
                nodeDelay = delayIt->second;
            }
            if (txIt != groupResults[i].nodeTxPackets.end()) {
                nodeTx = txIt->second;
            }
            if (rxIt != groupResults[i].nodeRxPackets.end()) {
                nodeRx = rxIt->second;
            }
            if (lostIt != groupResults[i].nodeLostPackets.end()) {
                nodeLost = lostIt->second;
            }
            
            double deliveryRatio = (nodeTx > 0) ? (double)nodeRx / nodeTx : 0;
            double effectiveLoad = lambda * config.nodeLoads[nodeId];
            
            file << "Group," << lambda << "," << nodeId << ","
                 << nodeThroughput << "," << nodeDelay << ","
                 << nodeTx << "," << nodeRx << "," << nodeLost << ","
                 << deliveryRatio << ","
                 << config.nodeLoads[nodeId] << "," << effectiveLoad << "\n";
        }
        lambda += config.lambdaStep;
    }
    
    file.close();
    std::cout << "Node statistics CSV written to: " << filename << std::endl;
}

// Реализация нового метода для записи результатов анализа четырьмя методами
void CsvWriter::WriteAnalysisWithMethods(const AnalysisResults& adHocAnalysis,
                                        const AnalysisResults& groupAnalysis,
                                        const std::string& filename) {
    std::ofstream file(filename);
    
    // Заголовок CSV файла
    file << "NetworkType,Lambda,ActualThroughput,ActualDelay,"
         << "MeanValueAnalysis,GlobalBalanceMethod,GordonNewellMethod,BuzenMethod,MeanValueDelay\n";
    
    // Записываем результаты для Ad-Hoc сети
    for (size_t i = 0; i < adHocAnalysis.lambdas.size(); ++i) {
        file << "AdHoc," << adHocAnalysis.lambdas[i] << ","
             << adHocAnalysis.actualThroughputs[i] << "," 
             << adHocAnalysis.actualDelays[i] << ","
             << adHocAnalysis.meanValueAnalysis[i] << ","
             << adHocAnalysis.globalBalanceMethod[i] << ","
             << adHocAnalysis.gordonNewellMethod[i] << ","
             << adHocAnalysis.buzenMethod[i] << ","
             << adHocAnalysis.meanValueDelay[i] << "\n";
    }
    
    // Записываем результаты для Group сети
    for (size_t i = 0; i < groupAnalysis.lambdas.size(); ++i) {
        file << "Group," << groupAnalysis.lambdas[i] << ","
             << groupAnalysis.actualThroughputs[i] << "," 
             << groupAnalysis.actualDelays[i] << ","
             << groupAnalysis.meanValueAnalysis[i] << ","
             << groupAnalysis.globalBalanceMethod[i] << ","
             << groupAnalysis.gordonNewellMethod[i] << ","
             << groupAnalysis.buzenMethod[i] << ","
             << groupAnalysis.meanValueDelay[i] << "\n";
    }
    
    file.close();
    std::cout << "Analysis with methods CSV written to: " << filename << std::endl;
}
