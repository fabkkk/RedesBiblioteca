1. A Estrutura Física e Lógica (Os Enlaces)
A rede é construída conectando três tecnologias diferentes:
- O Backbone (Point-to-Point): É a "espinha dorsal" da rede que interliga os sistemas. Ele conecta o Roteador Central (Nó 0) diretamente ao Access Point do Wi-Fi (Nó 1). Foi configurado com uma banda alta (1Gbps) e um atraso mínimo (2ms) para garantir um trânsito rápido entre os dois mundos (cabeado e sem fio).
- A Rede Cabeada Administrativa (CSMA): Simula uma rede local tradicional (Ethernet) operando a 100Mbps. O Roteador Central (Nó 0) faz parte desta rede, servindo de ponte. Nela também estão o Servidor de Acervo (Nó 2), o Servidor de Empréstimos (Nó 3) e os dois computadores do balcão de atendimento (Nós 4 e 5).
- A Rede Sem Fio dos Alunos (Wi-Fi): O Nó 1 atua como a antena (Access Point) irradiando o SSID "UFPA-Biblioteca". Os Nós 6 a 11 representam os notebooks e smartphones dos alunos conectados a essa rede. Eles possuem um modelo de mobilidade estático (ConstantPositionMobilityModel), indicando que os alunos estão parados nas mesas de estudo.

2. O Endereçamento e Roteamento
Para que um aluno na rede Wi-Fi consiga solicitar um arquivo do Servidor de Acervo na rede cabeada, a topologia foi dividida em três sub-redes:
- 10.1.1.0: Exclusiva para a comunicação direta no cabo Point-to-Point entre o Roteador Central e o Access Point.
- 10.1.2.0: Endereça as máquinas administrativas e os servidores (CSMA).
- 10.1.3.0: Distribui IPs para os dispositivos dos alunos no Wi-Fi.

O comando Ipv4GlobalRoutingHelper::PopulateRoutingTables () cria magicamente as tabelas de roteamento em todos os nós. Ele ensina ao Access Point como enviar pacotes do IP 10.1.3.X para a rede 10.1.2.X. 

3. O Modelo de Tráfego
A simulação comprova o funcionamento da rede executando dois tipos de transferência de dados:  
- CBR sobre UDP (Sistema de Empréstimos): Um dos computadores do balcão (Nó 4) envia um fluxo de pacotes com tamanho fixo (1024 bytes) a cada 0.1 segundos para o Servidor de Empréstimos (Nó 3). Isso simula o tráfego CBR (Constant Bit Rate), funcionando como um ping ou "batimento cardíaco" constante para manter a sessão do sistema da biblioteca ativa. Usa UDP porque é rápido e perdas esporádicas não derrubam a aplicação.
- FTP sobre TCP (Download de PDFs): O Servidor de Acervo (Nó 2) inicia o envio de um arquivo virtualmente infinito (configurado com MaxBytes igual a 0 na aplicação BulkSend) para o dispositivo de um aluno no Wi-Fi (Nó 6). O protocolo TCP entra em ação para garantir que nenhum pacote se perca no ar, ativando mecanismos de controle de congestionamento.
