
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>

using namespace ns3;

int main(int argc, char *argv[])
{
    // PAGE 1: Create 4 hosts and 2 routers
    NodeContainer hosts, routers;
    hosts.Create(4);
    routers.Create(2);


    // PAGE 1: Configure access links (10 Mbps, 2 ms)
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer d0 = p2p.Install(hosts.Get(0), routers.Get(0));
    NetDeviceContainer d1 = p2p.Install(hosts.Get(1), routers.Get(0));
    NetDeviceContainer d2 = p2p.Install(routers.Get(1), hosts.Get(2));
    NetDeviceContainer d3 = p2p.Install(routers.Get(1), hosts.Get(3));


    // PAGE 1: Configure bottleneck link (2 Mbps, 10 ms)
    // DropTail queue with maximum 50 packets
    p2p.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));

    p2p.SetQueue("ns3::DropTailQueue<Packet>",
                 "MaxSize", StringValue("50p"));

    NetDeviceContainer d4 =
        p2p.Install(routers.Get(0), routers.Get(1));


    // PAGE 1: Install Internet stack
    InternetStackHelper internet;
    internet.Install(hosts);
    internet.Install(routers);


    // PAGE 1: Assign IP addresses to all 5 links
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0 = address.Assign(d0);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i1 = address.Assign(d1);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i2 = address.Assign(d2);

    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i3 = address.Assign(d3);

    address.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i4 = address.Assign(d4);


    // PAGE 1: Enable global routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    // PAGE 2: TCP receiver (Host 3, port 5000)
    PacketSinkHelper sink("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 5000));

    ApplicationContainer sinkApp = sink.Install(hosts.Get(3));
    sinkApp.Start(Seconds(1));
    sinkApp.Stop(Seconds(20));


    // PAGE 2: TCP sender (Host 0 -> Host 3)
    BulkSendHelper tcp("ns3::TcpSocketFactory",
        InetSocketAddress(i3.GetAddress(1), 5000));

    tcp.SetAttribute("MaxBytes", UintegerValue(0));
    tcp.SetAttribute("SendSize", UintegerValue(1024));

    ApplicationContainer tcpApp = tcp.Install(hosts.Get(0));
    tcpApp.Start(Seconds(2));
    tcpApp.Stop(Seconds(19));


    // PAGE 2: UDP server (Host 2, port 6000)
    UdpServerHelper udpServer(6000);

    ApplicationContainer serverApp =
        udpServer.Install(hosts.Get(2));

    serverApp.Start(Seconds(1));
    serverApp.Stop(Seconds(20));


    // PAGE 2: UDP client (Host 1 -> Host 2)
    UdpClientHelper udp(i2.GetAddress(1), 6000);

    udp.SetAttribute("MaxPackets", UintegerValue(100000));
    udp.SetAttribute("PacketSize", UintegerValue(1024));

    // PAGE 2: UDP interval formula (1 Mbps)
    udp.SetAttribute("Interval",
        TimeValue(Seconds(1024 * 8.0 / 1e6)));

    ApplicationContainer udpApp = udp.Install(hosts.Get(1));
    udpApp.Start(Seconds(2));
    udpApp.Stop(Seconds(19));


    // PAGE 3: Install FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();


    // PAGE 4: Run simulation for 20 seconds
    Simulator::Stop(Seconds(20));
    Simulator::Run();


    // PAGE 3: Iterate over flow statistics
    mon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());

    auto stats = mon->GetFlowStats();

    for (auto flow : stats)
    {
        auto s = flow.second;
        auto t = classifier->FindFlow(flow.first);

        // Additional filtering: only TCP and UDP data flows
        if (t.destinationPort != 5000 &&
            t.destinationPort != 6000)
            continue;


        // PAGE 4: Throughput in Mbps
        double duration =
            (s.timeLastRxPacket -
             s.timeFirstTxPacket).GetSeconds();

        double throughput = duration > 0
            ? s.rxBytes * 8.0 / duration / 1e6 : 0;


        // PAGE 4: Packet loss percentage
        double loss = s.txPackets > 0
            ? 100.0 * (s.txPackets - s.rxPackets) / s.txPackets
            : 0;


        // PAGE 4: Average delay in milliseconds
        double delay = s.rxPackets > 0
            ? s.delaySum.GetSeconds() / s.rxPackets * 1000
            : 0;


        // PAGE 3-4: Display results
        std::cout << "\nFlow ID: " << flow.first
                  << "\nDestination Port: " << t.destinationPort
                  << "\nThroughput: " << throughput << " Mbps"
                  << "\nPacket Loss: " << loss << "%"
                  << "\nAverage Delay: " << delay << " ms\n";
    }


    // PAGE 4: Cleanup
    Simulator::Destroy();
    return 0;
}
