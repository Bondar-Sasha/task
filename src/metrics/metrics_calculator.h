#pragma once

#include "ns3/flow-monitor-module.h"
#include "simulation_metrics.h"
#include <vector>

using namespace ns3;

class MetricsCalculator {
public:
    static SimulationMetrics Calculate(Ptr<FlowMonitor> flowMonitor, 
                                      double simulationTime, 
                                      const std::vector<double>& nodeLoads);
};

