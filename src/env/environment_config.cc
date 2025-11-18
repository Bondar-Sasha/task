#include "environment_config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <stdexcept>

SimulationConfig::SimulationConfig()
    : simulationDuration(0.0),
      packetSize(0),
      numNodes(0),
      lambdaStart(0.0),
      lambdaEnd(0.0),
      lambdaStep(0.0),
      graphDpi(0),
      graphInterpolationPoints(0),
      bufferSize(3),
      maxPackets(100),
      nodeLoadMode("custom"),
      dataRateMbps(5.0),
      linkDelayMs(0.5),
      wifiStandard("80211a"),
      wifiMaxRange(100.0),
      gridDeltaX(30.0),
      gridDeltaY(30.0),
      gridWidth(3),
      startTimeMin(0.1),
      startTimeMax(1.0),
      udpServerPortStart(1000),
      udpClientPortStart(2000),
      buzenCustomersMultiplier(2.0),
      serviceRate(5.0),
      numGroups(3) {}

SimulationConfig EnvironmentConfig::Load(const std::string& filename) {
    SimulationConfig config;

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Configuration file not found: " + filename);
    }

    std::unordered_set<std::string> scalarKeys;
    std::unordered_set<int> nodeLoadKeys;
    std::unordered_set<int> nodeBufferKeys;

    std::string line;
    while (std::getline(file, line)) {
        auto commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line = Trim(line);
        if (line.empty()) {
            continue;
        }

        auto equalsPos = line.find('=');
        if (equalsPos == std::string::npos) {
            continue;
        }

        std::string key = Trim(line.substr(0, equalsPos));
        std::string value = Trim(line.substr(equalsPos + 1));

        if (!key.empty()) {
            ProcessConfigValue(config, key, value, scalarKeys, nodeLoadKeys, nodeBufferKeys);
        }
    }

    const std::vector<std::string> requiredScalarKeys = {
        "SIMULATION_DURATION",
        "PACKET_SIZE",
        "NUM_NODES",
        "LAMBDA_START",
        "LAMBDA_END",
        "LAMBDA_STEP",
        "GRAPH_DPI",
        "GRAPH_INTERPOLATION_POINTS"
    };

    for (const auto& requiredKey : requiredScalarKeys) {
        if (!scalarKeys.count(requiredKey)) {
            throw std::runtime_error("Missing required configuration key: " + requiredKey);
        }
    }

    if (config.numNodes == 0) {
        throw std::runtime_error("NUM_NODES must be greater than zero");
    }

    for (int nodeId : nodeLoadKeys) {
        if (nodeId < 0 || static_cast<uint32_t>(nodeId) >= config.numNodes) {
            throw std::runtime_error("NODE_" + std::to_string(nodeId) + "_LOAD is out of range for declared NUM_NODES");
        }
    }

    for (int nodeId : nodeBufferKeys) {
        if (nodeId < 0 || static_cast<uint32_t>(nodeId) >= config.numNodes) {
            throw std::runtime_error("NODE_" + std::to_string(nodeId) + "_BUFFER is out of range for declared NUM_NODES");
        }
    }

    if (nodeLoadKeys.size() != config.numNodes) {
        throw std::runtime_error("Expected NODE_X_LOAD entries for all nodes in range [0, " + std::to_string(config.numNodes - 1) + "]");
    }

    if (nodeBufferKeys.size() != config.numNodes) {
        throw std::runtime_error("Expected NODE_X_BUFFER entries for all nodes in range [0, " + std::to_string(config.numNodes - 1) + "]");
    }

    if (config.nodeLoads.size() != config.numNodes) {
        throw std::runtime_error("nodeLoads size mismatch with NUM_NODES");
    }

    if (config.nodeBuffers.size() != config.numNodes) {
        throw std::runtime_error("nodeBuffers size mismatch with NUM_NODES");
    }

    std::cout << "Config loaded from: " << filename << std::endl;
    return config;
}

void EnvironmentConfig::ProcessConfigValue(SimulationConfig& config,
                                           const std::string& key,
                                           const std::string& value,
                                           std::unordered_set<std::string>& scalarKeys,
                                           std::unordered_set<int>& nodeLoadKeys,
                                           std::unordered_set<int>& nodeBufferKeys) {
    if (key == "SIMULATION_DURATION") {
        config.simulationDuration = std::stod(value);
        scalarKeys.insert(key);
    } else if (key == "PACKET_SIZE") {
        config.packetSize = static_cast<uint32_t>(std::stoul(value));
        scalarKeys.insert(key);
    } else if (key == "NUM_NODES") {
        config.numNodes = static_cast<uint32_t>(std::stoul(value));
        scalarKeys.insert(key);
    } else if (key == "LAMBDA_START") {
        config.lambdaStart = std::stod(value);
        scalarKeys.insert(key);
    } else if (key == "LAMBDA_END") {
        config.lambdaEnd = std::stod(value);
        scalarKeys.insert(key);
    } else if (key == "LAMBDA_STEP") {
        config.lambdaStep = std::stod(value);
        scalarKeys.insert(key);
    } else if (key == "GRAPH_DPI") {
        config.graphDpi = std::stoi(value);
        scalarKeys.insert(key);
    } else if (key == "GRAPH_INTERPOLATION_POINTS") {
        config.graphInterpolationPoints = std::stoi(value);
        scalarKeys.insert(key);
    } else if (key.rfind("NODE_", 0) == 0 && key.find("_LOAD") != std::string::npos) {
        auto underscore1 = key.find('_');
        auto underscore2 = key.find('_', underscore1 + 1);
        if (underscore2 != std::string::npos) {
            int nodeId = std::stoi(key.substr(underscore1 + 1, underscore2 - underscore1 - 1));
            if (nodeId >= 0) {
                if (static_cast<size_t>(nodeId) >= config.nodeLoads.size()) {
                    config.nodeLoads.resize(static_cast<size_t>(nodeId) + 1, 0.0);
                    config.nodeBuffers.resize(config.nodeLoads.size(), 0);
                }
                config.nodeLoads[static_cast<size_t>(nodeId)] = std::stod(value);
                nodeLoadKeys.insert(nodeId);
            }
        }
    } else if (key.rfind("NODE_", 0) == 0 && key.find("_BUFFER") != std::string::npos) {
        auto underscore1 = key.find('_');
        auto underscore2 = key.find('_', underscore1 + 1);
        if (underscore2 != std::string::npos) {
            int nodeId = std::stoi(key.substr(underscore1 + 1, underscore2 - underscore1 - 1));
            if (nodeId >= 0) {
                if (static_cast<size_t>(nodeId) >= config.nodeBuffers.size()) {
                    config.nodeLoads.resize(static_cast<size_t>(nodeId) + 1, 0.0);
                    config.nodeBuffers.resize(static_cast<size_t>(nodeId) + 1, 0);
                }
                config.nodeBuffers[static_cast<size_t>(nodeId)] = static_cast<uint32_t>(std::stoul(value));
                nodeBufferKeys.insert(nodeId);
            }
        }
    }
}

std::string EnvironmentConfig::Trim(const std::string& value) {
    auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch); });
    auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}
