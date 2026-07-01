# README – Topologia e Funcionamento da Rede

## 1. Estrutura Física e Lógica da Rede

A rede é construída utilizando três tecnologias diferentes de enlace.

### Backbone (Point-to-Point)

É a espinha dorsal da rede, responsável por interligar os dois principais segmentos da infraestrutura.

- Conecta o **Roteador Central (Nó 0)** diretamente ao **Access Point (Nó 1)**.
- Banda configurada: **1 Gbps**.
- Atraso (Delay): **2 ms**.

Essa configuração garante uma comunicação rápida entre a rede cabeada e a rede sem fio.

### Rede Cabeada Administrativa (CSMA)

Simula uma rede Ethernet tradicional operando a **100 Mbps**.

Nela estão conectados:

- **Nó 0** – Roteador Central
- **Nó 2** – Servidor de Acervo
- **Nó 3** – Servidor de Empréstimos
- **Nó 4** – Computador do Balcão 1
- **Nó 5** – Computador do Balcão 2

O Roteador Central atua como ponte entre a rede administrativa e a rede sem fio.

### Rede Sem Fio dos Alunos (Wi-Fi)

O **Nó 1** atua como Access Point, transmitindo o SSID **"UFPA-Biblioteca"**.

Os dispositivos dos alunos são representados pelos:

- **Nó 6**
- **Nó 7**
- **Nó 8**
- **Nó 9**
- **Nó 10**
- **Nó 11**

Todos utilizam o modelo de mobilidade **ConstantPositionMobilityModel**, representando alunos sentados em suas mesas de estudo durante toda a simulação.

---

## 2. Endereçamento IP e Roteamento

Para permitir a comunicação entre os diferentes segmentos da rede, a topologia foi dividida em três sub-redes:

- **10.1.1.0/24** – Comunicação Point-to-Point entre o Roteador Central e o Access Point.
- **10.1.2.0/24** – Rede administrativa (CSMA), incluindo servidores e computadores do balcão.
- **10.1.3.0/24** – Rede Wi-Fi utilizada pelos dispositivos dos alunos.

O roteamento é configurado automaticamente através do comando:

```cpp
Ipv4GlobalRoutingHelper::PopulateRoutingTables();
```

Esse comando cria as tabelas de roteamento de todos os nós, permitindo, por exemplo, que um dispositivo da rede Wi-Fi (**10.1.3.X**) consiga acessar os servidores presentes na rede administrativa (**10.1.2.X**).

---

## 3. Modelo de Tráfego

A simulação valida o funcionamento da rede utilizando dois tipos distintos de comunicação.

### CBR sobre UDP (Sistema de Empréstimos)

Um dos computadores do balcão (**Nó 4**) envia continuamente pacotes ao **Servidor de Empréstimos (Nó 3)**.

**Características:**

- **Protocolo:** UDP
- **Tamanho do pacote:** 1024 bytes
- **Intervalo entre pacotes:** 0,1 segundo

Esse fluxo representa um tráfego **CBR (Constant Bit Rate)**, funcionando como um "batimento cardíaco" da aplicação para manter a sessão do sistema ativa.

O protocolo **UDP** foi escolhido por apresentar baixa latência e tolerar perdas ocasionais de pacotes sem comprometer o funcionamento da aplicação.

### FTP sobre TCP (Download de PDFs)

O **Servidor de Acervo (Nó 2)** realiza o envio de um arquivo virtualmente infinito para um dispositivo de aluno (**Nó 6**).

**Características:**

- **Protocolo:** TCP
- **Aplicação:** BulkSendApplication
- **Configuração:** `MaxBytes = 0`

Ao configurar `MaxBytes = 0`, a aplicação transmite dados continuamente durante toda a simulação.

O protocolo **TCP** garante a entrega confiável dos dados por meio de mecanismos como:

- Controle de congestionamento;
- Retransmissão de pacotes perdidos;
- Controle de fluxo.

Esse cenário simula o download de arquivos PDF do acervo digital da biblioteca pelos alunos conectados à rede Wi-Fi.
