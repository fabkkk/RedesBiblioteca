/*
 * Projeto: Simulacao de Rede da Biblioteca Central     
 * Disciplina: Rede de Computadores
 * Alunos: Fabio, Claudio e Luana
 * * Mini-Mundo e Trafego:
 * 1. FTP (TCP): Alunos baixando PDFs pesados do Acervo (BulkSend -> PacketSink)
 * 2. CBR (UDP): Balcao enviando telemetria unidirecional constante ao Servidor (OnOff -> PacketSink)
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
    p2pNodes.Create (2);

    NodeContainer csmaNodes;
    csmaNodes.Add (p2pNodes.Get (0)); 
    csmaNodes.Create (4); 

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (6); 
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
    
    // rede cabeada (roteador, acervo, emprestimos e balcoes)
    MobilityHelper mobilityCsma;
    Ptr<ListPositionAllocator> posCsma = CreateObject<ListPositionAllocator> ();
    posCsma->Add (Vector (30.0, 20.0, 0.0)); // Roteador (Centro)
    posCsma->Add (Vector (10.0, 20.0, 0.0)); // Acervo (Esquerda)
    posCsma->Add (Vector (30.0, 40.0, 0.0)); // Emprestimos (Abaixo do Roteador)
    posCsma->Add (Vector (10.0, 40.0, 0.0)); // Balcao 1 (Abaixo e Esquerda)
    posCsma->Add (Vector (50.0, 40.0, 0.0)); // Balcao 2 (Abaixo e Direita)
    mobilityCsma.SetPositionAllocator (posCsma);
    mobilityCsma.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityCsma.Install (csmaNodes); 

    // access point
    MobilityHelper mobilityAp;
    Ptr<ListPositionAllocator> posAp = CreateObject<ListPositionAllocator> ();
    posAp->Add (Vector (50.0, 20.0, 0.0)); // AP na mesma linha do Roteador
    mobilityAp.SetPositionAllocator (posAp);
    mobilityAp.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityAp.Install (wifiApNode);

    // alunos
    MobilityHelper mobilityAlunos;
    mobilityAlunos.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (65.0),
                                   "MinY", DoubleValue (5.0),
                                   "DeltaX", DoubleValue (18.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (2),
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
    Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), cbrPort));
    PacketSinkHelper packetSinkHelperUdp ("ns3::UdpSocketFactory", localAddress);
    ApplicationContainer udpSinkApps = packetSinkHelperUdp.Install (csmaNodes.Get (2)); 
    udpSinkApps.Start (Seconds (1.0));
    udpSinkApps.Stop (Seconds (10.0));

    OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (csmaInterfaces.GetAddress (2), cbrPort)));
    onoff.SetConstantRate (DataRate ("80kbps"), 1024); 
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
    ApplicationContainer tcpSourceApps = bulkSend.Install (csmaNodes.Get (1)); 
    tcpSourceApps.Start (Seconds (3.0));
    tcpSourceApps.Stop (Seconds (10.0));

    // ==========================================
    //                  METRICAS 
    // ==========================================

    pointToPoint.EnablePcapAll ("pcap-backbone");
    csma.EnablePcap ("pcap-lan", csmaDevices.Get (1), true); 
    phy.EnablePcap ("pcap-wifi", staDevices.Get (0));        

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

    monitor->SerializeToXmlFile ("biblioteca-estatisticas.xml", true, true);

    Simulator::Destroy ();
    return 0;
}