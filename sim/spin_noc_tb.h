#ifndef SPIN_NOC_TB_H
#define SPIN_NOC_TB_H

#include <systemc.h>
#include "../src/noc_pkg.h"

// ============================================================
// Módulo: SpinNocTb (Testbench)
// Descrição: Gerador de tráfego e monitor para a NoC SPIN.
//
//   Cenários de teste implementados:
//     1. HOST 0 -> HOST 7 (pior caso: 3 saltos pela árvore)
//        Caminho: R0 -> R4/R5 -> R3 -> HOST 7
//     2. HOST 2 -> HOST 5 (cruzamento entre subárvores)
//     3. HOST 0 -> HOST 1 (melhor caso: destino local, 0 saltos)
//
//   Para cada cenário, o testbench:
//     - Injeta o flit pela porta LOCAL do roteador folha
//     - Monitora a chegada no host destino
//     - Mede e imprime a latência (timestamp atual - timestamp do flit)
//     - Verifica integridade (payload e src_id corretos)
// ============================================================
SC_MODULE(SpinNocTb) {
    // ----- Sinais de controle -----
    sc_in<bool>  clk;
    sc_out<bool> rst;

    // ----- Portas de injeção de tráfego (TB -> NoC) -----
    sc_out<Flit> tb_in[8];
    sc_out<bool> tb_valid_in[8];
    sc_in<bool>  tb_ready_out[8]; // Backpressure da NoC

    // ----- Portas de recepção (NoC -> TB) -----
    sc_in<Flit>  tb_out[8];
    sc_in<bool>  tb_valid_out[8];
    sc_out<bool> tb_ready_in[8]; // TB aceita dado do host

    // ----- Threads do testbench -----
    SC_CTOR(SpinNocTb) {
        SC_THREAD(gerador_trafego);
        sensitive << clk.pos();

        SC_THREAD(monitor_recepcao);
        sensitive << clk.pos();
    }

    // Thread principal: injeta pacotes na NoC
    void gerador_trafego();

    // Thread de monitoramento: observa chegada nos destinos
    void monitor_recepcao();

    // Função auxiliar: injeta um flit e aguarda handshake
    void injeta_flit(int host_origem, int host_destino,
                     int dado, const std::string& descricao);

    // Função auxiliar: aguarda chegada em um host destino
    // com timeout (em ciclos) — retorna true se recebeu
    bool aguarda_recepcao(int host_destino, int timeout_ciclos,
                          Flit& flit_recebido);
};

#endif // SPIN_NOC_TB_H
