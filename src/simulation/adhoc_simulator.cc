#include "adhoc_simulator.h"
#include "../metrics/metrics_calculator.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>

using namespace ns3;

SimulationMetrics AdHocSimulator::Run(const SimulationConfig& config, double lambda) {
    std::cout << "=== Running Ad-Hoc Simulation (Lambda=" << lambda << ") ===" << std::endl;
    
    // Создаем 9 узлов (решетка 3x3)
    NodeContainer nodes;
    nodes.Create(config.numNodes);
    
    // Настраиваем WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", 
                                 "MaxRange", DoubleValue(config.wifiMaxRange));
    
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());
    
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);
    
    // Размещаем узлы в решетке 3x3
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue(0.0),
                                 "MinY", DoubleValue(0.0),
                                 "DeltaX", DoubleValue(config.gridDeltaX),
                                 "DeltaY", DoubleValue(config.gridDeltaY),
                                 "GridWidth", UintegerValue(3),  // Фиксированная ширина решетки 3
                                 "LayoutType", StringValue("RowFirst"));
    
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);
    
    // Устанавливаем стек протоколов
    InternetStackHelper internet;
    internet.Install(nodes);
    
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);
    
    // Настройка буферов для узлов - пропускаем, так как WiFi не поддерживает прямой доступ к TxQueue
    // Вместо этого используем параметры QoS
    Config::SetDefault("ns3::WifiMacQueue::MaxSize", 
                     QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, config.bufferSize)));
    
    Ptr<UniformRandomVariable> rv = CreateObject<UniformRandomVariable>();
    
    // Создаем приложения для каждого узла
    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        double nodeLambda = lambda * config.nodeLoads[i];
        if (nodeLambda > 0) {
            // Выбираем случайного получателя (не самого себя)
            uint32_t receiverIdx = rv->GetInteger(0, nodes.GetN() - 1);
            while (receiverIdx == i) {
                receiverIdx = rv->GetInteger(0, nodes.GetN() - 1);
            }
            
            uint16_t port = config.udpServerPortStart + i;
            UdpServerHelper server(port);
            ApplicationContainer serverApp = server.Install(nodes.Get(receiverIdx));
            serverApp.Start(Seconds(0.0));
            serverApp.Stop(Seconds(config.simulationDuration));
            
            UdpClientHelper client(interfaces.GetAddress(receiverIdx), port);
            // Убираем ограничение MaxPackets, чтобы нагрузка росла с Lambda
            // Количество пакетов = lambda * simulationDuration
            uint32_t totalPackets = static_cast<uint32_t>(nodeLambda * config.simulationDuration * 1.5);
            client.SetAttribute("MaxPackets", UintegerValue(totalPackets));
            client.SetAttribute("Interval", TimeValue(Seconds(1.0 / nodeLambda)));
            client.SetAttribute("PacketSize", UintegerValue(config.packetSize));
            
            ApplicationContainer clientApp = client.Install(nodes.Get(i));
            double startTime = rv->GetValue(config.startTimeMin, config.startTimeMax);
            clientApp.Start(Seconds(startTime));
            clientApp.Stop(Seconds(config.simulationDuration - 0.1));
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

