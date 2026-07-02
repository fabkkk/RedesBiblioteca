# Simulação de Rede da Biblioteca Central (ns-3)

Este repositório contém o código-fonte e a documentação para a simulação de uma infraestrutura de rede híbrida de uma Biblioteca Universitária, desenvolvida utilizando o simulador de redes **Network Simulator 3 (ns-3)**.

O projeto tem como objetivo principal analisar o comportamento do roteamento IP e a coexistência de tráfegos heterogêneos (TCP e UDP) em domínios de broadcast distintos, isolando as demandas administrativas das demandas acadêmicas.

## Equipe Desenvolvedora
* **Fábio**
* **Claudio**
* **Luana**

---

## Topologia e Arquitetura

A rede foi projetada utilizando uma topologia centralizada em torno de um **Roteador Central**, que interliga e isola dois subsistemas principais:

### 1. Sistema de Empréstimos (LAN Administrativa)
* **Meio Físico:** Rede Cabeada (Padrão Ethernet / IEEE 802.3).
* **Protocolo de Enlace:** CSMA (100 Mbps, 6560 ns de delay).
* **Nós:** 1 Servidor de Empréstimos, 2 Balcões de Atendimento e o Roteador.
* **Perfil de Tráfego:** As mensagens de telemetria e controle de status dos balcões para o servidor são enviadas através de um fluxo constante usando o protocolo **UDP** (aplicação `OnOff` gerando pacotes de 1024 bytes a 80 kbps). O foco neste segmento é a **baixa latência**.

### 2. Sistema de Acervo Digital (WLAN Acadêmica)
* **Meio Físico:** Rede Sem Fio (Padrão Wi-Fi / IEEE 802.11).
* **Nós:** 1 Servidor de Acervo (conectado via link P2P ao Roteador), 1 Access Point (AP) e 6 Nós Clientes (Alunos).
* **Perfil de Tráfego:** A transferência de arquivos pesados (PDFs acadêmicos) do servidor para os alunos é simulada via **TCP/FTP** (aplicação `BulkSend`). O protocolo TCP garante o controle de congestionamento e a confiabilidade na entrega de grandes volumes de dados.

---

## Pré-requisitos e Configuração do Ambiente

Para compilar e executar esta simulação, você precisará do ambiente Linux padrão para desenvolvimento no simulador:
* **Sistema Operacional:** Linux Ubuntu (recomendado) ou compatível.
* **Ferramentas:** Compilador C++ (GCC/G++), Python 3.
* **Simulador:** [ns-3](https://www.nsnam.org/) (versão 3.30 ou superior).
* **Análise Visual e de Pacotes (Opcional):** NetAnim e Wireshark.

---

## Como Executar

1. Clone este repositório para o diretório `scratch` ou `src` do seu ambiente de trabalho do ns-3.
2. Navegue até a pasta raiz do ns-3 pelo terminal.
3. Compile e execute o script C++ utilizando o `ns3` (ou `waf`, dependendo da sua versão do simulador):

```bash
./ns3 run scratch/biblioteca_sim.cc
```
*(Substitua `biblioteca_sim.cc` pelo nome exato do arquivo `.cc` salvo na pasta `scratch`)*

---

## Métricas e Resultados

Ao final dos 10 segundos de simulação, o script gerará automaticamente os seguintes arquivos no diretório de execução:

* **`biblioteca-estatisticas.xml`**: Arquivo gerado pelo `FlowMonitor`, contendo dados detalhados sobre vazão (throughput), atraso (delay), jitter e perda de pacotes para cada fluxo (TCP e UDP).
* **`biblioteca-animacao.xml`**: Arquivo de trace XML pronto para ser aberto no software **NetAnim**, permitindo a visualização gráfica da topologia, posicionamento dos nós e o trânsito interativo dos pacotes.
* **Arquivos `.pcap`**: Traces de captura de rede prontos para análise profunda no **Wireshark**:
  * `pcap-backbone-X-X.pcap`
  * `pcap-lan-X-X.pcap` (Focado no tráfego CSMA)
  * `pcap-wifi-X-X.pcap` (Focado no download acadêmico)

---
*Projeto desenvolvido para a disciplina de Redes de Computadores.*
