#pragma once

#include "../env/environment_config.h"
#include "../metrics/simulation_metrics.h"

class GroupSimulator {
public:
    static SimulationMetrics Run(const SimulationConfig& config, double lambda);
};

