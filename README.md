# NoC SPIN em SystemC — 8 Roteadores (Fat-Tree 2 Níveis)

## Visão Geral

Este projeto implementa uma **Rede em Chip (NoC)** baseada na topologia **SPIN (Scalable, Programmable, Integrated Network)** usando **SystemC**.

A topologia é uma **Fat-Tree de 2 níveis** com:
- **4 Roteadores Folha** (IDs 0–3): conectados a 2 hosts locais cada
- **4 Roteadores Raiz** (IDs 4–7): interconectam as folhas
- **8 Hosts** (IDs 0–7): terminais finais de injeção/recepção de tráfego

Cada roteador folha conecta-se a dois roteadores raiz distintos (via portas `UP_0` e `UP_1`), garantindo dois caminhos alternativos para qualquer destino fora da subárvore local.
```
Hosts conectados a cada folha:

  [LEAF 0]       [LEAF 1]       [LEAF 2]       [LEAF 3]
  /      \       /      \       /      \       /      \
 H0      H1     H2      H3     H4      H5     H6      H7
```
```
Conexões raiz ↔ folha (cada raiz serve exatamente 2 folhas):

  R4 ──DOWN_0──► R0    R4 ──DOWN_1──► R2
  R5 ──DOWN_0──► R0    R5 ──DOWN_1──► R3
  R6 ──DOWN_0──► R1    R6 ──DOWN_1──► R2
  R7 ──DOWN_0──► R1    R7 ──DOWN_1──► R3
```
> Consequência: qualquer par folha–folha tem exatamente um roteador raiz em comum, garantindo um caminho de 3 saltos (folha → raiz → folha) para toda comunicação entre subárvores distintas.

**Diagrama de conexões UP/DOWN:**
```
R0 UP_0  ↔  R4 DOWN_0      R1 UP_0  ↔  R6 DOWN_0
R0 UP_1  ↔  R5 DOWN_0      R1 UP_1  ↔  R7 DOWN_0
R2 UP_0  ↔  R4 DOWN_1      R3 UP_0  ↔  R5 DOWN_1
R2 UP_1  ↔  R6 DOWN_1      R3 UP_1  ↔  R7 DOWN_1
```

---

## Estrutura do Projeto

```
spin_noc/
├── Makefile
├── README.md
├── sim/
│   ├── main.cpp           ← sc_main: instanciação, conexão, VCD e controle da simulação
│   ├── spin_noc_tb.cpp    ← Gerador de tráfego + monitor de recepção
│   └── spin_noc_tb.h
└── src/
    ├── noc_pkg.h          ← Tipos globais: Flit, PortID, constantes
    ├── fifo_buffer.h/.cpp ← Buffer de entrada (fila circular, capacidade 4)
    ├── routing_logic.h/.cpp← Algoritmo SPIN de decisão de rota
    ├── arbiter.h/.cpp     ← Árbitro Round-Robin anti-starvation
    ├── crossbar.h/.cpp    ← Matriz de comutação (chaveamento de flits)
    ├── router.h           ← Roteador completo (instancia todos os submódulos)
    └── spin_noc_top.h     ← Top-level: 8 roteadores interconectados
```

---

## Pré-requisitos

- **GCC** ≥ 7.0 com suporte a C++14
- **SystemC** 2.3.x instalado (testado com 2.3.3)
- *(Opcional)* **GTKWave** para visualização das formas de onda VCD

---

## Como Compilar e Executar

### 1. Configure o caminho do SystemC

```bash
export SYSTEMC_HOME=/usr/local/systemc-2.3.3   # ajuste para o seu caminho
```

### 2. Compile

```bash
make
```

### 3. Execute a simulação

```bash
make run
# ou diretamente:
./sim_noc
```

### 4. Visualize as formas de onda

```bash
make wave
# ou:
gtkwave simulation_waveform.vcd
```

---

## Cenários de Teste Implementados

| # | Origem | Destino | Caminho | Tipo |
|---|--------|---------|---------|------|
| 1 | HOST 0 | HOST 7  | R0 → R4/R5 → R3 → H7 | **Pior caso** (3 saltos) |
| 2 | HOST 2 | HOST 5  | R1 → R4/R5 → R2 → H5 | Cruzamento de subárvores |
| 3 | HOST 0 | HOST 1  | R0 → H1               | **Melhor caso** (local) |

---

## Algoritmo de Roteamento SPIN

**Roteadores Folha (IDs 0–3):**
- Se `(dest_id / 2) == router_id` → destino é **local**:
  - `dest_id` par → porta `LOCAL_0`
  - `dest_id` ímpar → porta `LOCAL_1`
- Caso contrário → **sobe** pela árvore com toggle Round-Robin entre `UP_0` e `UP_1`

**Roteadores Raiz (IDs 4–7):**
- `dest_id` 0–3 → porta `DOWN_0` (subárvore esquerda)
- `dest_id` 4–7 → porta `DOWN_1` (subárvore direita)

---

## Componentes Principais

| Módulo | Arquivo | Descrição |
|--------|---------|-----------|
| `FifoBuffer` | fifo_buffer.h/.cpp | Buffer de entrada com handshake valid/ready, capacidade 4 flits |
| `RoutingLogic` | routing_logic.h/.cpp | Cálculo de rota SPIN com toggle para balanceamento |
| `Arbiter` | arbiter.h/.cpp | Árbitro Round-Robin com ponteiro circular anti-starvation |
| `Crossbar` | crossbar.h/.cpp | Matriz de comutação baseada nos grants dos árbitros |
| `Router` | router.h | Roteador completo: 6 FIFOs + 6 lógicas + 6 árbitros + 1 crossbar |
| `SpinNocTop` | spin_noc_top.h | Top-level com 8 roteadores interconectados na topologia Fat-Tree |
| `SpinNocTb` | spin_noc_tb.h/.cpp | Testbench: injeção, monitoramento, medição de latência |