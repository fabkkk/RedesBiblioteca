/*
 * Projeto: Simulação de Rede da Biblioteca Central
 * Disciplina: Rede de Computadores
 * Alunos: Fabio e equipe
 * * Mini-Mundo:
 * A rede simula a infraestrutura de uma Biblioteca Universitária contendo:
 * - Servidores na rede cabeada (Acervo Digital e Sistema de Empréstimos)
 * - Computadores do balcão de atendimento na rede cabeada
 * - Dispositivos móveis de alunos conectados via Wi-Fi
 * * Tráfego Modelado:
 * 1. FTP (TCP): Alunos fazendo download de PDFs pesados do Servidor de Acervo.
 * 2. CBR (UDP): Computadores do balcão enviando batimentos (ping constante) para o Servidor de Empréstimos.
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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BibliotecaCentralSim");

int main (int argc, char *argv[]) {
    // Habilitando logs para acompanharmos o tráfego no terminal
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // ==========================================
    // 1. CRIAÇÃO DOS NÓS (Total: 12 Nós)
    // ==========================================
    
    NodeContainer p2pNodes;
    p2pNodes.Create (2); // Nó 0 (Roteador Central), Nó 1 (Access Point)

    NodeContainer csmaNodes;
    csmaNodes.Add (p2pNodes.Get (0)); // Nó 0 (Roteador atuando na LAN cabeada)
    csmaNodes.Create (4); // Nó 2 (Serv. Acervo), Nó 3 (Serv. Empréstimos), Nós 4 e 5 (Balcão)

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (6); // Nós 6 a 11 (Alunos no Wi-Fi)
    NodeContainer wifiApNode = p2pNodes.Get (1); // Nó 1 (Access Point)

    // ==========================================
    // 2. CONFIGURAÇÃO DOS ENLACES (Links)
    // ==========================================

    // Link Point-to-Point (Backbone entre Roteador Central e AP)
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer p2pDevices = pointToPoint.Install (p2pNodes);

    // Link CSMA (Rede Cabeada Local)
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
    NetDeviceContainer csmaDevices = csma.Install (csmaNodes);

    // Link Wi-Fi (Rede dos Alunos)
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());

    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("UFPA-Biblioteca");
    
    // Configurando dispositivos dos alunos
    mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
    NetDeviceContainer staDevices = wifi.Install (phy, mac, wifiStaNodes);

    // Configurando dispositivo do AP
    mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
    NetDeviceContainer apDevices = wifi.Install (phy, mac, wifiApNode);

    // ==========================================
    // 3. MOBILIDADE (Obrigatório para Wi-Fi)
    // ==========================================
    
    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiStaNodes);
    mobility.Install (wifiApNode);

    // ==========================================
    // 4. PILHA DE PROTOCOLOS E ENDEREÇAMENTO
    // ==========================================
    
    InternetStackHelper stack;
    stack.Install (csmaNodes);
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;

    // IP P2P
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign (p2pDevices);

    // IP CSMA
    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces = address.Assign (csmaDevices);

    // IP Wi-Fi
    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces = address.Assign (staDevices);
    address.Assign (apDevices);

    // Roteamento Global
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // ==========================================
    // 5. TRÁFEGO 1: CBR / UDP (Balcão -> Servidor Empréstimos)
    // ==========================================
    
    // Servidor UDP no Nó 3 (Servidor de Empréstimos da rede CSMA, índice 2 do container CSMA)
    uint16_t cbrPort = 9;
    UdpEchoServerHelper echoServer (cbrPort);
    ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (2)); 
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    // Cliente UDP no Nó 4 (Computador do Balcão, índice 3 do container CSMA)
    UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (2), cbrPort);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (100)); // Tráfego constante
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (csmaNodes.Get (3));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));

    // ==========================================
    // 6. TRÁFEGO 2: FTP / TCP (Servidor Acervo -> Aluno Wi-Fi)
    // ==========================================
    
    // Sink TCP no Nó 6 (Primeiro aluno no Wi-Fi, índice 0)
    uint16_t ftpPort = 50000;
    Address sinkAddress (InetSocketAddress (staInterfaces.GetAddress (0), ftpPort));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ftpPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (0));
    sinkApps.Start (Seconds (1.0));
    sinkApps.Stop (Seconds (10.0));

    // Bulk Send TCP (Transferência pesada) no Nó 2 (Servidor Acervo, índice 1 do CSMA)
    BulkSendHelper bulkSend ("ns3::TcpSocketFactory", sinkAddress);
    bulkSend.SetAttribute ("MaxBytes", UintegerValue (0)); // 0 significa envio ilimitado (simula arquivo muito grande)
    ApplicationContainer sourceApps = bulkSend.Install (csmaNodes.Get (1));
    sourceApps.Start (Seconds (3.0));
    sourceApps.Stop (Seconds (10.0));

    // ==========================================
    // 7. EXECUÇÃO DA SIMULAÇÃO
    // ==========================================
    
    Simulator::Stop (Seconds (10.0));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}