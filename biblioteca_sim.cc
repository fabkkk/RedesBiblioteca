/*
 * Projeto: Simulacao de Rede da Biblioteca Central
 * Disciplina: Rede de Computadores
 * Alunos: Fabio, Claudio e Luana
 * * Mini-Mundo e Trafego:
 * FTP (TCP): Alunos baixando PDFs do Acervo (BulkSend -> PacketSink)
 * CBR (UDP): Balcao enviando telemetria unidirecional constante ao Servidor (OnOff -> PacketSink)
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BibliotecaCentralSim");

int main (int argc, char *argv[]) {
    // ==========================================
    //             CRIACAO DOS NOS
    // ==========================================
    
    NodeContainer p2pNodes;
    p2pNodes.Create (2); // No 0 (Roteador Central), No 1 (Access Point)

    NodeContainer csmaNodes;
    csmaNodes.Add (p2pNodes.Get (0)); 
    // csmaNodes contera:
    // Indice 0 -> No Global 0 (Roteador Central)
    // Indice 1 -> No Global 2 (Servidor Acervo)
    // Indice 2 -> No Global 3 (Servidor Emprestimos)
    // Indice 3 -> No Global 4 (Balcao 1)
    // Indice 4 -> No Global 5 (Balcao 2)
    csmaNodes.Create (4); 

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (6); // Nos Globais 6 a 11 (Alunos no Wi-Fi)
    NodeContainer wifiApNode = p2pNodes.Get (1); 

    // ==========================================
    //         CONFIGURACAO DOS ENLACES
    // ==========================================

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer p2pDevices = pointToPoint.Install (p2pNodes);

    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
    NetDeviceContainer csmaDevices = csma.Install (csmaNodes);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());

    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("UFPA-Biblioteca");
    
    mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
    NetDeviceContainer staDevices = wifi.Install (phy, mac, wifiStaNodes);

    mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
    NetDeviceContainer apDevices = wifi.Install (phy, mac, wifiApNode);

    // ==========================================
    //          POSICIONAMENTO DOS NOS
    // ==========================================
    
    // posicoes dos nos
    MobilityHelper mobilityInfra;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (20.0, 10.0, 0.0)); // No 0: Roteador Central
    positionAlloc->Add (Vector (20.0, 20.0, 0.0)); // No 1: Access Point
    positionAlloc->Add (Vector (10.0, 10.0, 0.0)); // No 2: Serv. Acervo
    positionAlloc->Add (Vector (30.0, 10.0, 0.0)); // No 3: Serv. Emprestimos
    positionAlloc->Add (Vector (10.0, 0.0, 0.0));  // No 4: Balcao 1
    positionAlloc->Add (Vector (30.0, 0.0, 0.0));  // No 5: Balcao 2
    
    mobilityInfra.SetPositionAllocator (positionAlloc);
    mobilityInfra.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityInfra.Install (csmaNodes); 
    mobilityInfra.Install (wifiApNode);

    // posicoes dos alunos
    MobilityHelper mobilityAlunos;
    mobilityAlunos.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (15.0),
                                   "MinY", DoubleValue (25.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (5.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
    mobilityAlunos.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityAlunos.Install (wifiStaNodes);

    // ==========================================
    //        PROTOCOLOS E ENDERECAMENTO
    // ==========================================
    
    InternetStackHelper stack;
    stack.Install (csmaNodes);
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;

    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign (p2pDevices);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces = address.Assign (csmaDevices);

    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces = address.Assign (staDevices);
    address.Assign (apDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // ==========================================
    //               TRAFEGO UDP 
    // ==========================================
    
    uint16_t cbrPort = 9;
    
    // Servidor UDP
    Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), cbrPort));
    PacketSinkHelper packetSinkHelperUdp ("ns3::UdpSocketFactory", localAddress);
    ApplicationContainer udpSinkApps = packetSinkHelperUdp.Install (csmaNodes.Get (2)); 
    udpSinkApps.Start (Seconds (1.0));
    udpSinkApps.Stop (Seconds (10.0));

    // Cliente UDP
    OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (csmaInterfaces.GetAddress (2), cbrPort)));
    onoff.SetConstantRate (DataRate ("80kbps"), 1024); // Simula 1 pacote de 1024 bytes a cada ~0.1s
    ApplicationContainer udpClientApps = onoff.Install (csmaNodes.Get (3));
    udpClientApps.Start (Seconds (2.0));
    udpClientApps.Stop (Seconds (10.0));

    // ==========================================
    //            TRAFEGO FTP / TCP 
    // ==========================================
    
    uint16_t ftpPort = 50000;
    Address sinkAddress (InetSocketAddress (staInterfaces.GetAddress (0), ftpPort));
    PacketSinkHelper packetSinkHelperTcp ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ftpPort));
    ApplicationContainer tcpSinkApps = packetSinkHelperTcp.Install (wifiStaNodes.Get (0));
    tcpSinkApps.Start (Seconds (1.0));
    tcpSinkApps.Stop (Seconds (10.0));

    BulkSendHelper bulkSend ("ns3::TcpSocketFactory", sinkAddress);
    bulkSend.SetAttribute ("MaxBytes", UintegerValue (0)); 
    ApplicationContainer tcpSourceApps = bulkSend.Install (csmaNodes.Get (1)); // No Global 2 (Indice 1)
    tcpSourceApps.Start (Seconds (3.0));
    tcpSourceApps.Stop (Seconds (10.0));

    // ==========================================
    //                  METRICAS 
    // ==========================================

    // Wireshark
    pointToPoint.EnablePcapAll ("pcap-backbone");
    csma.EnablePcap ("pcap-lan", csmaDevices.Get (1), true); // Grampeia o Servidor de Acervo
    phy.EnablePcap ("pcap-wifi", staDevices.Get (0));        // Grampeia o Aluno que faz download

    // FlowMonitor para estatisticas
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    // ==========================================
    //                  NETANIM
    // ==========================================
    
    AnimationInterface anim ("biblioteca-animacao.xml");
    anim.SetMaxPktsPerTraceFile (99999999); 
    
    anim.UpdateNodeDescription (0, "Roteador_Central");
    anim.UpdateNodeDescription (1, "Access_Point");
    anim.UpdateNodeDescription (2, "Serv_Acervo(FTP)");
    anim.UpdateNodeDescription (3, "Serv_Emprest(UDP)");
    anim.UpdateNodeDescription (4, "Balcao_1");
    anim.UpdateNodeDescription (5, "Balcao_2");
    
    for (uint32_t i = 0; i < wifiStaNodes.GetN (); ++i) {
        std::string name = (i == 0) ? "Aluno_Download" : "Aluno_Ouvinte_" + std::to_string(i);
        anim.UpdateNodeDescription (wifiStaNodes.Get (i)->GetId (), name);
    }
    // ==========================================
    //                 SIMULACAO
    // ==========================================
    
    Simulator::Stop (Seconds (10.0));
    Simulator::Run ();

    // Exporta as metricas do FlowMonitor para um XML antes de destruir o simulador
    monitor->SerializeToXmlFile ("biblioteca-estatisticas.xml", true, true);

    Simulator::Destroy ();
    return 0;
}
