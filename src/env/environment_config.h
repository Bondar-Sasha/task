#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

struct SimulationConfig {
    double simulationDuration;
    uint32_t packetSize;
    uint32_t numNodes;
    
    double lambdaStart;
    double lambdaEnd;
    double lambdaStep;
    
    std::vector<double> nodeLoads;
    std::vector<uint32_t> nodeBuffers;
    
    int graphDpi;
    int graphInterpolationPoints;
    
    // Дефолтные значения для отсутствующих параметров
    uint32_t bufferSize;
    uint32_t maxPackets;
    std::string nodeLoadMode;
    double dataRateMbps;
    double linkDelayMs;
    std::string wifiStandard;
    double wifiMaxRange;
    double gridDeltaX;
    double gridDeltaY;
    uint32_t gridWidth;
    double startTimeMin;
    double startTimeMax;
    uint16_t udpServerPortStart;
    uint16_t udpClientPortStart;
    double buzenCustomersMultiplier;
    double serviceRate;
    uint32_t numGroups;
    
    SimulationConfig();
};

class EnvironmentConfig {
public:
    static SimulationConfig Load(const std::string& filename);

private:
    static void ProcessConfigValue(SimulationConfig& config,
                                   const std::string& key,
                                   const std::string& value,
                                   std::unordered_set<std::string>& scalarKeys,
                                   std::unordered_set<int>& nodeLoadKeys,
                                   std::unordered_set<int>& nodeBufferKeys);
    static std::string Trim(const std::string& value);
};