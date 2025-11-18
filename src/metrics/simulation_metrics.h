#pragma once

#include <cstdint>
#include <map>

struct SimulationMetrics {
    double throughput;
    double delay;
    double packetLoss;
    double load;
    uint32_t txPackets;
    uint32_t rxPackets;
    uint32_t lostPackets;
    uint32_t droppedPackets;
    double jitter;
    double avgHopCount;
    double txBytes;
    double rxBytes;
    double simulationTime;
    std::map<uint32_t, double> nodeThroughput;
    std::map<uint32_t, double> nodeDelay;
    std::map<uint32_t, uint32_t> nodeTxPackets;
    std::map<uint32_t, uint32_t> nodeRxPackets;
    std::map<uint32_t, uint32_t> nodeLostPackets;
    
    SimulationMetrics();
};

