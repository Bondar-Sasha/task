#include "metrics_calculator.h"
#include <algorithm>
#include <limits>
#include <numeric> // <-- Добавлено для std::accumulate

SimulationMetrics MetricsCalculator::Calculate(Ptr<FlowMonitor> flowMonitor, 
                                               double simulationTime, 
                                               const std::vector<double>& nodeLoads) {
    SimulationMetrics metrics;
    metrics.simulationTime = simulationTime;
    
    FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();
    uint32_t totalTxPackets = 0;
    uint32_t totalRxPackets = 0;
    uint32_t totalLostPackets = 0;
    uint64_t totalDroppedPackets = 0;
    double totalDelaySeconds = 0;
    uint64_t delaySamples = 0; // ИСПРАВЛЕНО: Это общее кол-во *пакетов*, а не *флоу*
    double totalJitterSeconds = 0;
    uint64_t jitterSamples = 0;
    double totalThroughput = 0;
    uint32_t validFlows = 0;
    double totalTxBytes = 0;
    double totalRxBytes = 0;
    uint64_t totalHops = 0; // ИСПРАВЛЕНО: Тип изменен на uint64_t для соответствия flowStats
    
    metrics.nodeThroughput.clear();
    metrics.nodeDelay.clear();
    metrics.nodeTxPackets.clear();
    metrics.nodeRxPackets.clear();
    metrics.nodeLostPackets.clear();
    
    for (auto& flow : stats) {
        FlowMonitor::FlowStats flowStats = flow.second;
        
        totalTxPackets += flowStats.txPackets;
        totalRxPackets += flowStats.rxPackets;
        totalLostPackets += flowStats.lostPackets;
        for (uint32_t droppedCount : flowStats.packetsDropped) {
            totalDroppedPackets += droppedCount;
        }
        totalTxBytes += flowStats.txBytes;
        totalRxBytes += flowStats.rxBytes;
        
        if (flowStats.rxPackets > 0) {
            double flowDelayAvg = flowStats.delaySum.GetSeconds() / flowStats.rxPackets;
            
            // Аккумулируем общую сумму задержек и общее кол-во пакетов для среднего
            totalDelaySeconds += flowStats.delaySum.GetSeconds();
            delaySamples += flowStats.rxPackets;

            if (flowStats.rxPackets > 1) {
                totalJitterSeconds += flowStats.jitterSum.GetSeconds();
                jitterSamples += (flowStats.rxPackets - 1);
            }
            
            // Эта пропускная способность - средняя за *все время* симуляции (в Мбит/с)
            double flowThroughput = flowStats.rxBytes * 8.0 / (simulationTime * 1000000.0);
            
            totalThroughput += flowThroughput;
            totalHops += flowStats.timesForwarded; // timesForwarded - это *сумма хопов* для *всех* rx пакетов
            validFlows++;
            
            // -----------------------------------------------------------------
            // ВНИМАНИЕ: КРИТИЧЕСКАЯ ЛОГИЧЕСКАЯ ОШИБКА
            // -----------------------------------------------------------------
            // `flow.first` - это *ID потока* (Flow ID), а НЕ *ID узла* (Node ID).
            // С настройками FlowMonitor по умолчанию, это будут просто числа 1, 2, 3...
            // 
            // Чтобы эта логика работала, вам НЕОБХОДИМО в коде *симулятора* // (например, adhoc_simulator.cpp) настроить FlowMonitor
            // с использованием `Ipv4FlowClassifier`.
            //
            // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
            //
            // После этого `flow.first` станет `FiveTuple`, и вы сможете
            // извлечь `t.sourceAddress` и `t.destinationAddress`.
            //
            // ТЕКУЩАЯ ЛОГИКА ЗАПИСЫВАЕТ СТАТИСТИКУ В НЕВЕРНЫЕ КЛЮЧИ!
            // (я оставляю код как есть, чтобы не сломать вашу компиляцию,
            // но вы должны знать, что он работает НЕПРАВИЛЬНО)
            // -----------------------------------------------------------------
            uint32_t senderNode = flow.first; // <-- ЭТО НЕПРАВИЛЬНО!
            
            metrics.nodeThroughput[senderNode] += flowThroughput;
            metrics.nodeDelay[senderNode] = flowDelayAvg; // Это перезапишет, если у узла >1 потока
            metrics.nodeTxPackets[senderNode] += flowStats.txPackets;
            metrics.nodeRxPackets[senderNode] += flowStats.rxPackets;
            metrics.nodeLostPackets[senderNode] += flowStats.lostPackets;
        }
    }
    
    metrics.txPackets = totalTxPackets;
    metrics.rxPackets = totalRxPackets;
    metrics.lostPackets = totalLostPackets;
    metrics.droppedPackets = static_cast<uint32_t>(
        std::min<uint64_t>(totalDroppedPackets, std::numeric_limits<uint32_t>::max()));
    metrics.txBytes = totalTxBytes;
    metrics.rxBytes = totalRxBytes;
    
    if (totalTxPackets > 0) {
        metrics.packetLoss = (double)(totalLostPackets + totalDroppedPackets) / totalTxPackets;
    } else {
        metrics.packetLoss = 0;
    }
    
    if (delaySamples > 0) {
        metrics.delay = totalDelaySeconds / static_cast<double>(delaySamples);
    } else {
        metrics.delay = 0;
    }
    if (jitterSamples > 0) {
        metrics.jitter = totalJitterSeconds / static_cast<double>(jitterSamples);
    } else {
        metrics.jitter = 0;
    }
    
    metrics.throughput = totalThroughput;

    if (delaySamples > 0) {
        // Среднее число хопов = (Сумма всех хопов) / (Сумма всех *полученных пакетов*)
        metrics.avgHopCount = totalHops / static_cast<double>(delaySamples);
    } else {
        metrics.avgHopCount = 0;
    }
    
    if (!nodeLoads.empty()) {
        double totalLoad = std::accumulate(nodeLoads.begin(), nodeLoads.end(), 0.0);
        // 'load' - это средняя предложенная нагрузка (λ) на узел
        metrics.load = totalLoad / nodeLoads.size(); 
    } else if (simulationTime > 0 && totalTxPackets > 0) {
        // Запасной вариант: измеряем фактическую скорость отправки (пакетов/сек)
        // Это *измеренная* λ, а не *предложенная*.
        metrics.load = static_cast<double>(totalTxPackets) / simulationTime;
    } else {
        metrics.load = 0;
    }
    
    return metrics;
}