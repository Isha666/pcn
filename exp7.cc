
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
    // 1. Link delay
    std::string delay = "2ms";

    CommandLine cmd(__FILE__);
    cmd.AddValue("delay", "Link delay", delay);
    cmd.Parse(argc, argv);

    // 2. Create nodes
    NodeContainer hosts;
    hosts.Create(2);

    // 3. Point-to-point link
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue(delay));

    NetDeviceContainer dev01 =
        p2p.Install(hosts.Get(0), hosts.Get(1));

    // 4. Internet stack
    InternetStackHelper internet;
    internet.Install(hosts);

    // 5. IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer iface01 = ipv4.Assign(dev01);

    // 6. UDP Server 1 - Port 5000
    UdpServerHelper server1(5000);
    ApplicationContainer sApp1 = server1.Install(hosts.Get(1));
    sApp1.Start(Seconds(1));
    sApp1.Stop(Seconds(10));

    // 7. UDP Client 1 - 1 Mbps
    UdpClientHelper client1(iface01.GetAddress(1), 5000);

    client1.SetAttribute("MaxPackets", UintegerValue(100000));
    client1.SetAttribute("PacketSize", UintegerValue(512));
    client1.SetAttribute("Interval",
        TimeValue(Seconds(512 * 8.0 / 1e6)));

    ApplicationContainer cApp1 = client1.Install(hosts.Get(0));
    cApp1.Start(Seconds(2));
    cApp1.Stop(Seconds(10));

    // 8. UDP Server 2 - Port 6000
    UdpServerHelper server2(6000);
    ApplicationContainer sApp2 = server2.Install(hosts.Get(1));
    sApp2.Start(Seconds(1));
    sApp2.Stop(Seconds(10));

    // 9. UDP Client 2 - 500 Kbps
    UdpClientHelper client2(iface01.GetAddress(1), 6000);

    client2.SetAttribute("MaxPackets", UintegerValue(100000));
    client2.SetAttribute("PacketSize", UintegerValue(512));
    client2.SetAttribute("Interval",
        TimeValue(Seconds(512 * 8.0 / 500000)));

    ApplicationContainer cApp2 = client2.Install(hosts.Get(0));
    cApp2.Start(Seconds(2));
    cApp2.Stop(Seconds(10));

    // 10. PCAP tracing
    p2p.EnablePcapAll("udp-exp7");

    // 11. FlowMonitor
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();

    // 12. Run simulation
    Simulator::Stop(Seconds(10));
    Simulator::Run();

    // 13. Throughput calculation
    mon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> cl =
        DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());

    for (const auto &kv : mon->GetFlowStats())
    {
        const auto &s = kv.second;
        auto t = cl->FindFlow(kv.first);

        if (t.destinationPort != 5000 &&
            t.destinationPort != 6000)
            continue;

        double dur =
            (s.timeLastRxPacket -
             s.timeFirstTxPacket).GetSeconds();

        double throughput =
            (s.rxPackets > 0 && dur > 0)
            ? (s.rxBytes * 8.0 / dur / 1e6)
            : 0.0;

        std::cout << "\nPort: " << t.destinationPort
                  << "\nThroughput: " << throughput
                  << " Mbps\n";
    }

    Simulator::Destroy();
    return 0;
}
