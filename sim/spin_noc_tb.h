#ifndef SPIN_NOC_TB_H
#define SPIN_NOC_TB_H

// ============================================================
// sim/spin_noc_tb.h — Testbench da NoC SPIN
//
// Módulo: SpinNocTb
// Descrição: Gerador de tráfego e monitor de recepção para a
//            NoC SPIN Fat-Tree com 8 roteadores e 8 hosts.
//
// Divisão de responsabilidades (exclusividade de drivers):
//
//   gerador_trafego  → ÚNICO escritor de: tb_in[], tb_valid_in[], rst
//   monitor_recepcao → ÚNICO escritor de: tb_ready_in[]
//
// Cenários de teste implementados:
//   1. HOST 0 -> HOST 7 (pior caso:  3 saltos, subárvores distintas)
//      Caminho: R0 -> R4/R5 -> R3 -> HOST 7
//   2. HOST 2 -> HOST 5 (cruzamento de subárvores)
//      Caminho: R1 -> R4/R5 -> R2 -> HOST 5
//   3. HOST 0 -> HOST 1 (melhor caso: destino local, 0 saltos externos)
//      Caminho: R0 -> HOST 1
//
// Para cada cenário, o testbench:
//   - Injeta o flit pela porta LOCAL do roteador folha de origem
//   - Aguarda confirmação de handshake (valid+ready) no lado TX
//   - O monitor independente detecta chegada no host destino
//   - Mede e imprime a latência (sc_time_stamp() - flit.timestamp)
//   - Verifica integridade do campo dest_id
//
// Protocolo de handshake:
//   TX  (TB -> NoC): valid_in=1, dado presente; handshake quando
//                    ready_out=1 no flanco positivo do clock.
//   RX  (NoC -> TB): valid_out=1 indica dado disponível; o monitor
//                    levanta ready_in=1 por exatamente 1 ciclo de
//                    clock para confirmar a leitura (pulso).
// ============================================================

#include <systemc.h>
#include "../src/noc_pkg.h"

SC_MODULE(SpinNocTb) {

    // ----- Sinais de controle globais -----
    sc_in<bool>  clk;
    sc_out<bool> rst;

    // ----- Portas de injeção de tráfego (TB -> NoC) -----
    // Escritas EXCLUSIVAMENTE por gerador_trafego
    sc_out<Flit> tb_in[8];
    sc_out<bool> tb_valid_in[8];
    sc_in<bool>  tb_ready_out[8]; // Backpressure da NoC (somente leitura no TB)

    // ----- Portas de recepção (NoC -> TB) -----
    // tb_ready_in[] é escrito EXCLUSIVAMENTE por monitor_recepcao
    sc_in<Flit>  tb_out[8];
    sc_in<bool>  tb_valid_out[8];
    sc_out<bool> tb_ready_in[8]; // Pulso de ACK: exclusivo de monitor_recepcao

    // ----- Registro de threads -----
    SC_CTOR(SpinNocTb) {
        // Thread do gerador: sensível à borda positiva do clock,
        // mas usa wait() explícito internamente — a sensitivity list
        // aqui serve apenas para o kick inicial do kernel SystemC.
        SC_THREAD(gerador_trafego);
        sensitive << clk.pos();

        // Thread do monitor: idem — loop infinito com wait() interno.
        SC_THREAD(monitor_recepcao);
        sensitive << clk.pos();
    }

    // ----- Declarações dos métodos -----

    // Thread principal de injeção de tráfego
    // Drivers: rst, tb_in[], tb_valid_in[]
    void gerador_trafego();

    // Thread de monitoramento contínuo de recepção
    // Drivers: tb_ready_in[]
    void monitor_recepcao();

    // Auxiliar de injeção: monta o pacote, espera handshake TX e limpa sinais
    // Chamado apenas de dentro de gerador_trafego
    // Drivers: tb_in[host_origem], tb_valid_in[host_origem]
    void injeta_flit(int host_origem, int host_destino,
                     int dado, const std::string& descricao);
};

#endif // SPIN_NOC_TB_H