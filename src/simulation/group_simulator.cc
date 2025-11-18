#include "group_simulator.h"
#include "../metrics/metrics_calculator.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
#include <vector>

using namespace ns3;

SimulationMetrics GroupSimulator::Run(const SimulationConfig& config, double lambda) {
    std::cout << "=== Running Group Simulation (Lambda=" << lambda << ") ===" << std::endl;
    
    // Создаем 3 группы по 3 узла (всего 9 узлов)
    const uint32_t numGroups = 3;
    const uint32_t nodesPerGroup = 3;
    
    std::vector<NodeContainer> groups;
    NodeContainer allNodes;
    NodeContainer masterNodes;  // Главные узлы в каждой группе
    
    // Создаем группы
    for (uint32_t i = 0; i < numGroups; ++i) {
        NodeContainer group;
        group.Create(nodesPerGroup);
        groups.push_back(group);
        allNodes.Add(group);
        
        // Первый узел в группе будет главным
        masterNodes.Add(group.Get(0));
    }
    
    // Устанавливаем стек протоколов
    InternetStackHelper internet;
    internet.Install(allNodes);
    
    // Создаем соединения внутри групп (CSMA)
    std::vector<NetDeviceContainer> groupDevices;
    std::vector<Ipv4InterfaceContainer> groupInterfaces;
    
    // Настраиваем размер буфера для CSMA устройств
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", 
                     QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, config.bufferSize)));
    
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(config.dataRateMbps * 1000000)));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(config.linkDelayMs)));
    
    Ipv4AddressHelper ipv4;
    
    // Создаем локальные сети для каждой группы
    for (uint32_t i = 0; i < numGroups; ++i) {
        NetDeviceContainer groupDevice = csma.Install(groups[i]);
        groupDevices.push_back(groupDevice);
        
        std::string base = "10.1." + std::to_string(i+1) + ".0";
        ipv4.SetBase(base.c_str(), "255.255.255.0");
        groupInterfaces.push_back(ipv4.Assign(groupDevice));
    }
    
    // Создаем соединения между главными узлами (Point-to-Point)
    NetDeviceContainer p2pDevices;
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate(config.dataRateMbps * 2000000)));  // Удвоенная скорость для магистрали
    p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(config.linkDelayMs / 2)));  // Меньшая задержка для магистрали
    
    // Соединяем главные узлы в кольцо
    for (uint32_t i = 0; i < numGroups; ++i) {
        uint32_t nextGroup = (i + 1) % numGroups;
        NetDeviceContainer link = p2p.Install(masterNodes.Get(i), masterNodes.Get(nextGroup));
        p2pDevices.Add(link);
    }
    
    // Назначаем IP-адреса для магистральной сети
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces = ipv4.Assign(p2pDevices);
    
    // Настраиваем маршрутизацию
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    // Создаем приложения для обмена данными
    Ptr<UniformRandomVariable> rv = CreateObject<UniformRandomVariable>();
    uint32_t globalNodeId = 0;
    
    for (uint32_t groupIdx = 0; groupIdx < numGroups; ++groupIdx) {
        NodeContainer& group = groups[groupIdx];
        
        for (uint32_t nodeIdx = 0; nodeIdx < nodesPerGroup; ++nodeIdx) {
            if (globalNodeId >= config.numNodes) break;
            
            double nodeLambda = lambda * config.nodeLoads[globalNodeId];
            if (nodeLambda > 0) {
                uint32_t targetGroupIdx = groupIdx;
                
                // Для главных узлов (индекс 0) - общение с другими группами
                if (nodeIdx == 0) {
                    targetGroupIdx = (groupIdx + 1) % numGroups;  // Общение с соседней группой
                }
                
                // Для обычных узлов - общение внутри группы
                uint32_t receiverIdx = nodeIdx == 0 ? 0 : (rv->GetInteger(1, nodesPerGroup - 1));  // Главный узел общается с главным, обычные - с обычными
                
                if (targetGroupIdx == groupIdx && receiverIdx == nodeIdx) {
                    receiverIdx = (receiverIdx + 1) % nodesPerGroup;
                    if (receiverIdx == 0) receiverIdx = 1;  // Избегаем главного узла для обычных узлов
                }
                
                uint16_t port = config.udpClientPortStart + globalNodeId;
                UdpServerHelper server(port);
                ApplicationContainer serverApp = server.Install(groups[targetGroupIdx].Get(receiverIdx));
                serverApp.Start(Seconds(0.0));
                serverApp.Stop(Seconds(config.simulationDuration));
                
                UdpClientHelper client;
                
                if (targetGroupIdx == groupIdx) {
                    // Внутригрупповое общение
                    client = UdpClientHelper(groupInterfaces[groupIdx].GetAddress(receiverIdx), port);
                } else {
                    // Межгрупповое общение (через главные узлы)
                    client = UdpClientHelper(groupInterfaces[targetGroupIdx].GetAddress(receiverIdx), port);
                }
                
                // Убираем ограничение MaxPackets, чтобы нагрузка росла с Lambda
                // Количество пакетов = lambda * simulationDuration
                uint32_t totalPackets = static_cast<uint32_t>(nodeLambda * config.simulationDuration * 1.5);
                client.SetAttribute("MaxPackets", UintegerValue(totalPackets));
                client.SetAttribute("Interval", TimeValue(Seconds(1.0 / nodeLambda)));
                client.SetAttribute("PacketSize", UintegerValue(config.packetSize));
                
                ApplicationContainer clientApp = client.Install(group.Get(nodeIdx));
                double startTime = rv->GetValue(config.startTimeMin, config.startTimeMax);
                clientApp.Start(Seconds(startTime));
                clientApp.Stop(Seconds(config.simulationDuration - 0.1));
            }
            globalNodeId++;
        }
    }
    
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();
    
    Simulator::Stop(Seconds(config.simulationDuration));
    Simulator::Run();
    
    SimulationMetrics metrics = MetricsCalculator::Calculate(monitor, config.simulationDuration, config.nodeLoads);
    
    Simulator::Destroy();
    
   
    return metrics;
}

