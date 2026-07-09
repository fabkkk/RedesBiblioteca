// importa os modulos do ns3 que preparam a rede e a simulacao
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

// define o nome do log para facilitar a busca por erros durante a execucao
NS_LOG_COMPONENT_DEFINE ("BibliotecaCentralSim");

// funcao principal onde a configuracao da rede e iniciada
int main (int argc, char *argv[]) {
    // cria dois nos para fazer uma conexao direta ponto a ponto entre roteadores
    NodeContainer p2pNodes;
    p2pNodes.Create (2);

    // cria a rede local da biblioteca e inclui o roteador principal para dar acesso
    NodeContainer csmaNodes;
    csmaNodes.Add (p2pNodes.Get (0)); 
    csmaNodes.Create (4); 

    // cria os dispositivos dos alunos que usarao a rede sem fio
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (6); 
    // define um dos nos do roteador como o ponto de acesso do wifi
    NodeContainer wifiApNode = p2pNodes.Get (1); 

    // ajusta a velocidade do link principal para garantir alta capacidade de transmissao
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer p2pDevices = pointToPoint.Install (p2pNodes);

    // configura a rede cabeada da biblioteca definindo velocidade e atraso do canal
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
    NetDeviceContainer csmaDevices = csma.Install (csmaNodes);

    // prepara o canal fisico por onde o sinal de wifi vai ser transmitido
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());

    // configura o wifi para usar um gerenciador de conexao padrao
    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

    // define o nome da rede wifi que sera transmitida no ambiente
    WifiMacHelper mac;
    Ssid ssid = Ssid ("UFPA-Biblioteca");
    
    // configura os dispositivos dos alunos para buscar e conectar nessa rede wifi
    mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
    NetDeviceContainer staDevices = wifi.Install (phy, mac, wifiStaNodes);

    // configura o ponto de acesso para distribuir o sinal com o nome escolhido
    mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
    NetDeviceContainer apDevices = wifi.Install (phy, mac, wifiApNode);

    // define posicoes fixas para os computadores cabeados ja que sao maquinas de mesa
    MobilityHelper mobilityCsma;
    Ptr<ListPositionAllocator> posCsma = CreateObject<ListPositionAllocator> ();
    posCsma->Add (Vector (30.0, 20.0, 0.0));
    posCsma->Add (Vector (10.0, 20.0, 0.0));
    posCsma->Add (Vector (30.0, 40.0, 0.0));
    posCsma->Add (Vector (10.0, 40.0, 0.0));
    posCsma->Add (Vector (50.0, 40.0, 0.0));
    mobilityCsma.SetPositionAllocator (posCsma);
    mobilityCsma.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityCsma.Install (csmaNodes); 

    // posiciona o ponto de acesso wifi em uma coordenada especifica do cenario
    MobilityHelper mobilityAp;
    Ptr<ListPositionAllocator> posAp = CreateObject<ListPositionAllocator> ();
    posAp->Add (Vector (50.0, 20.0, 0.0));
    mobilityAp.SetPositionAllocator (posAp);
    mobilityAp.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityAp.Install (wifiApNode);

    // distribui os alunos em formato de grade para simular as mesas de estudo
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

    // instala a pilha de protocolos de internet nos nos para permitir a comunicacao
    InternetStackHelper stack;
    stack.Install (csmaNodes);
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);

    // atribui uma faixa de enderecos ip para a conexao principal ponto a ponto
    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign (p2pDevices);

    // atribui uma segunda faixa de ips para a rede cabeada interna da biblioteca
    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces = address.Assign (csmaDevices);

    // define uma terceira faixa de ips exclusiva para os usuarios do wifi
    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces = address.Assign (staDevices);
    address.Assign (apDevices);

    // gera as tabelas de roteamento automaticamente para os nos saberem como se comunicar
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // configura um servidor na rede cabeada para receber os dados das aplicacoes
    uint16_t cbrPort = 9;
    Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), cbrPort));
    PacketSinkHelper packetSinkHelperUdp ("ns3::UdpSocketFactory", localAddress);
    ApplicationContainer udpSinkApps = packetSinkHelperUdp.Install (csmaNodes.Get (2)); 
    udpSinkApps.Start (Seconds (1.0));
    udpSinkApps.Stop (Seconds (10.0));

    // configura um terminal para enviar dados constantes simulando o sistema de emprestimos
    OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (csmaInterfaces.GetAddress (2), cbrPort)));
    onoff.SetConstantRate (DataRate ("80kbps"), 1024); 
    ApplicationContainer udpClientApps = onoff.Install (csmaNodes.Get (3));
    udpClientApps.Start (Seconds (2.0));
    udpClientApps.Stop (Seconds (10.0));

    // prepara o dispositivo de um aluno no wifi para receber transferencias de arquivos
    uint16_t tcpPort = 50000;
    Address sinkAddress (InetSocketAddress (staInterfaces.GetAddress (0), tcpPort));
    PacketSinkHelper packetSinkHelperTcp ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
    ApplicationContainer tcpSinkApps = packetSinkHelperTcp.Install (wifiStaNodes.Get (0));
    tcpSinkApps.Start (Seconds (1.0));
    tcpSinkApps.Stop (Seconds (10.0));

    // configura o servidor para enviar um grande volume de dados para o aluno no wifi
    BulkSendHelper bulkSend ("ns3::TcpSocketFactory", sinkAddress);
    bulkSend.SetAttribute ("MaxBytes", UintegerValue (0)); 
    ApplicationContainer tcpSourceApps = bulkSend.Install (csmaNodes.Get (1)); 
    tcpSourceApps.Start (Seconds (3.0));
    tcpSourceApps.Stop (Seconds (10.0));

    // ativa a captura de pacotes nas redes para permitir a analise do trafego depois
    pointToPoint.EnablePcapAll ("pcap-backbone");
    csma.EnablePcap ("pcap-lan", csmaDevices.Get (1), true); 
    phy.EnablePcap ("pcap-wifi", staDevices.Get (0));        

    // adiciona um monitor de fluxo para coletar metricas de desempenho da rede
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    // cria o arquivo de animacao para visualizar a simulacao em interface grafica
    AnimationInterface anim ("biblioteca-animacao.xml");
    anim.SetMaxPktsPerTraceFile (99999999); 
    
    // adiciona rotulos e CORES aos nos principais para facilitar a identificacao na animacao
    anim.UpdateNodeDescription (0, "Roteador_Central");
    anim.UpdateNodeColor (0, 255, 0, 0); // Vermelho

    anim.UpdateNodeDescription (1, "Access_Point");
    anim.UpdateNodeColor (1, 0, 200, 255); // Ciano

    anim.UpdateNodeDescription (2, "Serv_Acervo(TCP)");
    anim.UpdateNodeColor (2, 150, 0, 220); // Roxo

    anim.UpdateNodeDescription (3, "Serv_Emprest(UDP)");
    anim.UpdateNodeColor (3, 150, 0, 220); // Roxo

    anim.UpdateNodeDescription (4, "Balcao_1");
    anim.UpdateNodeColor (4, 255, 140, 0); // Laranja

    anim.UpdateNodeDescription (5, "Balcao_2");
    anim.UpdateNodeColor (5, 255, 140, 0); // Laranja
    
    // nomeia e colore cada aluno no wifi para diferenciar quem faz download dos demais
    for (uint32_t i = 0; i < wifiStaNodes.GetN (); ++i) {
        std::string name = (i == 0) ? "Aluno_Download" : "Aluno_Ouvinte_" + std::to_string(i);
        uint32_t nodeId = wifiStaNodes.Get(i)->GetId();
        anim.UpdateNodeDescription (nodeId, name);
        anim.UpdateNodeColor (nodeId, 0, 70, 255); // Azul
    }
    
    // define o tempo total da simulacao para dez segundos e inicia a execucao
    Simulator::Stop (Seconds (10.0));
    Simulator::Run ();

    // exporta as estatisticas de trafego para um arquivo xml para analise posterior
    monitor->SerializeToXmlFile ("biblioteca-estatisticas.xml", true, true);

    // finaliza a simulacao e libera a memoria utilizada pelo sistema
    Simulator::Destroy ();
    return 0;
}
