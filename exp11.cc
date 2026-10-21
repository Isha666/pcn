
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>

using namespace ns3;

int main(int argc, char *argv[])
{
    // PAGE 1: Create nodes
    NodeContainer apNode, staNodes, zigbeeNodes;
    apNode.Create(1);
    staNodes.Create(2);
    zigbeeNodes.Create(2);


    // PAGE 2: Wi-Fi channel and PHY
    YansWifiChannelHelper wifiChannel =
        YansWifiChannelHelper::Default();

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    WifiMacHelper wifiMac;
    Ssid ssid("campus-wifi");


    // PAGE 2: Install Wi-Fi AP
    wifiMac.SetType("ns3::ApWifiMac",
                    "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice =
        wifi.Install(wifiPhy, wifiMac, apNode);


    // PAGE 2: Install Wi-Fi stations
    wifiMac.SetType("ns3::StaWifiMac",
                    "Ssid", SsidValue(ssid));

    NetDeviceContainer staDevices =
        wifi.Install(wifiPhy, wifiMac, staNodes);


    // PAGE 3: Wi-Fi mobility
    MobilityHelper mobility;

    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(5.0),
        "GridWidth", UintegerValue(2),
        "LayoutType", StringValue("RowFirst"));

    // Change 1.0 to 5.0 or 10.0
    mobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds",
        RectangleValue(Rectangle(-100, 100, -100, 100)),
        "Speed",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1.0]"));

    mobility.Install(staNodes);


    // PAGE 3: Stationary AP
    mobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");

    mobility.Install(apNode);


    // PAGE 3: Install Zigbee-like LR-WPAN
    LrWpanHelper lrWpan;

    NetDeviceContainer lrDevices =
        lrWpan.Install(zigbeeNodes);

    lrWpan.AssociateToPan(lrDevices, 10);


    // PAGE 3: Install 6LoWPAN
    SixLowPanHelper sixlowpan;

    NetDeviceContainer sixDevices =
        sixlowpan.Install(lrDevices);


    // PAGE 3: Zigbee mobility
    MobilityHelper zigMobility;

    zigMobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(2.0),
        "DeltaY", DoubleValue(2.0),
        "GridWidth", UintegerValue(2),
        "LayoutType", StringValue("RowFirst"));

    zigMobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");

    zigMobility.Install(zigbeeNodes);


    // PAGE 1: Internet stack for Wi-Fi
    InternetStackHelper internet;
    internet.Install(apNode);
    internet.Install(staNodes);


    // IPv6-only Internet stack for Zigbee
    InternetStackHelper internet6;
    internet6.SetIpv4StackInstall(false);
    internet6.Install(zigbeeNodes);


    // PAGE 1: Wi-Fi IPv4 addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer apIface =
        ipv4.Assign(apDevice);

    Ipv4InterfaceContainer staIface =
        ipv4.Assign(staDevices);


    // PAGE 3: Zigbee IPv6 addresses
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(
        Ipv6Address("2001:1::"),
        Ipv6Prefix(64));

    Ipv6InterfaceContainer zigIface =
        ipv6.Assign(sixDevices);


    // PAGE 2: Wi-Fi UDP receiver on AP
    UdpServerHelper wifiServer(5000);

    ApplicationContainer wifiServerApp =
        wifiServer.Install(apNode.Get(0));

    wifiServerApp.Start(Seconds(1));
    wifiServerApp.Stop(Seconds(20));


    // PAGE 2: Zigbee UDP receiver on Sensor 1
    PacketSinkHelper zigSink(
        "ns3::UdpSocketFactory",
        Inet6SocketAddress(
            Ipv6Address::GetAny(), 6000));

    ApplicationContainer zigSinkApp =
        zigSink.Install(zigbeeNodes.Get(1));

    zigSinkApp.Start(Seconds(1));
    zigSinkApp.Stop(Seconds(20));


    // PAGE 2: Wi-Fi UDP senders on stations
    UdpClientHelper wifiClient(
        apIface.GetAddress(0), 5000);

    wifiClient.SetAttribute(
        "MaxPackets", UintegerValue(100000));

    wifiClient.SetAttribute(
        "PacketSize", UintegerValue(512));

    wifiClient.SetAttribute(
        "Interval",
        TimeValue(Seconds(512 * 8.0 / 1e6)));

    ApplicationContainer wifiClientApps =
        wifiClient.Install(staNodes);

    wifiClientApps.Start(Seconds(2));
    wifiClientApps.Stop(Seconds(19));


    // PAGE 2: Zigbee UDP sender on Sensor 0
    OnOffHelper zigClient(
        "ns3::UdpSocketFactory",
        Inet6SocketAddress(
            zigIface.GetAddress(1, 1), 6000));

    zigClient.SetConstantRate(
        DataRate("20kbps"), 60);

    ApplicationContainer zigClientApp =
        zigClient.Install(zigbeeNodes.Get(0));

    zigClientApp.Start(Seconds(2));
    zigClientApp.Stop(Seconds(19));


    // PAGE 3: FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();


    // PAGE 4: Run simulation
    Simulator::Stop(Seconds(20));
    Simulator::Run();

    mon->CheckForLostPackets();


    // PAGE 3: IPv4 flow classifier
    auto classifier =
        DynamicCast<Ipv4FlowClassifier>(
            fmh.GetClassifier());


    // PAGE 4: Wi-Fi performance metrics
    for (auto flow : mon->GetFlowStats())
    {
        auto s = flow.second;
        auto t = classifier->FindFlow(flow.first);

        if (t.destinationPort != 5000)
            continue;

        double duration =
            (s.timeLastRxPacket -
             s.timeFirstTxPacket).GetSeconds();

        double throughput = duration > 0
            ? s.rxBytes * 8.0 / duration / 1e6 : 0;

        double loss = s.txPackets > 0
            ? 100.0 * (s.txPackets - s.rxPackets)
                / s.txPackets : 0;

        double delay = s.rxPackets > 0
            ? s.delaySum.GetSeconds()
                / s.rxPackets * 1000 : 0;

        double pdr = s.txPackets > 0
            ? 100.0 * s.rxPackets / s.txPackets : 0;

        std::cout
            << "\nWi-Fi Flow: " << flow.first
            << "\nThroughput: " << throughput << " Mbps"
            << "\nLoss: " << loss << "%"
            << "\nDelay: " << delay << " ms"
            << "\nPDR: " << pdr << "%\n";
    }


    // Zigbee received bytes
    Ptr<PacketSink> zigReceived =
        DynamicCast<PacketSink>(
            zigSinkApp.Get(0));

    std::cout << "\nZigbee Bytes Received: "
              << zigReceived->GetTotalRx() << "\n";


    // PAGE 4: Cleanup
    Simulator::Destroy();
    return 0;
}
