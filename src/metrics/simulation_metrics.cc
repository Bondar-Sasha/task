#include "simulation_metrics.h"

SimulationMetrics::SimulationMetrics()
    : throughput(0), delay(0), packetLoss(0), load(0),
      txPackets(0), rxPackets(0), lostPackets(0), droppedPackets(0),
      jitter(0), avgHopCount(0), txBytes(0), rxBytes(0), simulationTime(0) {}

