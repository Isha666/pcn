#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>

using namespace ns3;

int main(int argc, char *argv[])
{
    // 1. Select TCP variant
    std::string tcp = "ns3::TcpCubic";

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcp", "TCP Variant", tcp);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       StringValue(tcp));

    // 2. Create nodes
    NodeContainer vehicles, rsu, server;
    vehicles.Create(5);
    rsu.Create(1);
    server.Create(1);

    // 3. Wi-Fi setup
    YansWifiChannelHelper channel =
        YansWifiChannelHelper::Default();

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue("HtMcs7"),
        "ControlMode", StringValue("OfdmRate6Mbps"));

    WifiMacHelper mac;
    Ssid ssid("VANET");

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid));
    NetDeviceContainer vehicleDevices =
        wifi.Install(phy, mac, vehicles);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));
    NetDeviceContainer rsuDevice =
        wifi.Install(phy, mac, rsu);

    // 4. Mobility
    MobilityHelper mobility;

    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(5.0),
        "GridWidth", UintegerValue(5),
        "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel(
        "ns3::ConstantVelocityMobilityModel");
    mobility.Install(vehicles);

    for (uint32_t i = 0; i < 5; i++)
    {
        vehicles.Get(i)
            ->GetObject<ConstantVelocityMobilityModel>()
            ->SetVelocity(Vector(15.0, 0.0, 0.0));
    }

    mobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");
    mobility.Install(rsu);
    mobility.Install(server);

    // 5. RSU to server link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate",
                           StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay",
                            StringValue("2ms"));

    NetDeviceContainer p2pDevices =
        p2p.Install(rsu.Get(0), server.Get(0));

    // 6. Internet stack
    NodeContainer all;
    all.Add(vehicles);
    all.Add(rsu);
    all.Add(server);

    InternetStackHelper internet;
    internet.Install(all);

    // 7. IP addressing
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(vehicleDevices);
    address.Assign(rsuDevice);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer serverInterface =
        address.Assign(p2pDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 8. TCP server
    PacketSinkHelper sink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 8080));

    ApplicationContainer sinkApp =
        sink.Install(server.Get(0));
    sinkApp.Start(Seconds(0));
    sinkApp.Stop(Seconds(20));

    // 9. TCP clients on vehicles
    BulkSendHelper source(
        "ns3::TcpSocketFactory",
        InetSocketAddress(
            serverInterface.GetAddress(1), 8080));

    source.SetAttribute("MaxBytes", UintegerValue(0));
    source.SetAttribute("SendSize", UintegerValue(1024));

    ApplicationContainer clientApps =
        source.Install(vehicles);
    clientApps.Start(Seconds(1));
    clientApps.Stop(Seconds(19));

    // 10. FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(20));
    Simulator::Run();

    monitor->CheckForLostPackets();

    auto classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowmon.GetClassifier());

    double sum = 0, sumSquares = 0;
    int count = 0;

    for (auto flow : monitor->GetFlowStats())
    {
        auto tuple = classifier->FindFlow(flow.first);

        // Only forward TCP traffic, not ACKs
        if (tuple.destinationPort != 8080)
            continue;

        auto s = flow.second;

        double duration =
            (s.timeLastRxPacket -
             s.timeFirstTxPacket).GetSeconds();

        double throughput = duration > 0
            ? s.rxBytes * 8.0 / duration / 1e6 : 0;

        double loss = s.txPackets > 0
            ? 100.0 * (s.txPackets - s.rxPackets)
              / s.txPackets : 0;

        double delay = s.rxPackets > 0
            ? s.delaySum.GetSeconds() /
              s.rxPackets * 1000 : 0;

        std::cout << "\nFlow " << flow.first
                  << "\nThroughput: " << throughput << " Mbps"
                  << "\nLoss: " << loss << "%"
                  << "\nDelay: " << delay << " ms\n";

        sum += throughput;
        sumSquares += throughput * throughput;
        count++;
    }

    double fairness = sumSquares > 0
        ? (sum * sum) / (count * sumSquares) : 0;

    std::cout << "\nFairness Index: "
              << fairness << "\n";

    Simulator::Destroy();
    return 0;
}

/*
mon->SerializeToXmlFile("flow-results.xml"
, true, true);
 1. Copy script into scratch directory
cp my_script.cc scratch/my_script.cc
# 2. Build the NS-3 project
./ns3 build
# 3. Run default simulation
./ns3 run scratch/my_script
# 4. Run with custom command-line arguments
./ns3 run "scratch/my_script --bw=5Mbps --delay=20ms --q=100"
# 5. Enable module logging during execution
NS_LOG="UdpClient=level_info" ./ns3 run scratch/my_script*/