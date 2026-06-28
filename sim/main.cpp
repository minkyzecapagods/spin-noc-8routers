// ============================================================
// sim/main.cpp — Ponto de entrada da simulação NoC SPIN
//
// AUDITORIA (sem alterações necessárias):
//   main.cpp está correto. Ele instancia SpinNocTop e SpinNocTb
//   separadamente e interconecta ambos via sinais intermediários
//   sig_tb_*[], sem nenhum driver duplicado no nível do top-level.
//
//   Os sinais sig_tb_ready_in[] são sc_signal<bool> — cada um
//   possui EXATAMENTE um driver (tb.tb_ready_in[i], escrito
//   exclusivamente por monitor_recepcao após as correções do
//   spin_noc_tb.cpp). Os sinais sig_tb_ready_in[] são usados
//   apenas como sc_in<bool> pelo lado da NoC (SpinNocTop), o
//   que não constitui um segundo driver.
//
//   Duração da simulação: 2000 ns (200 ciclos de 10 ns).
//   Arquivo VCD: simulation_waveform.vcd
// ============================================================

#include <systemc.h>
#include <iostream>
#include "../src/spin_noc_top.h"
#include "spin_noc_tb.h"

int sc_main(int argc, char* argv[]) {
    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "   NoC SPIN em SystemC — Implementação Fat-Tree 8 Roteadores" << std::endl;
    std::cout << "   Disciplina: Organização de Computadores" << std::endl;
    std::cout << "============================================================" << std::endl;

    // ----------------------------------------------------------
    // 1. Criar sinais de clock (10 ns, duty cycle 50%) e reset
    // ----------------------------------------------------------
    sc_clock clk("clk", 10, SC_NS);
    sc_signal<bool> rst("rst");

    // ----------------------------------------------------------
    // 2. Instanciar o Top-Level da NoC e o Testbench
    // ----------------------------------------------------------
    SpinNocTop noc("SPIN_NoC_8_Roteadores");
    SpinNocTb  tb("Testbench");

    // ----------------------------------------------------------
    // 3. Sinais de interconexão entre TB e NoC (8 hosts)
    //
    //    Auditoria de drivers:
    //      sig_tb_in[i]        → driver: tb.tb_in[i]      (gerador_trafego)
    //      sig_tb_valid_in[i]  → driver: tb.tb_valid_in[i](gerador_trafego)
    //      sig_tb_ready_out[i] → driver: noc.tb_ready_out[i](Router/FIFO)
    //      sig_tb_out[i]       → driver: noc.tb_out[i]    (Router/FIFO)
    //      sig_tb_valid_out[i] → driver: noc.tb_valid_out[i](Router/FIFO)
    //      sig_tb_ready_in[i]  → driver: tb.tb_ready_in[i](monitor_recepcao)
    //    Cada sinal tem EXATAMENTE um driver — sem E115.
    // ----------------------------------------------------------
    sc_signal<Flit> sig_tb_in[8];
    sc_signal<bool> sig_tb_valid_in[8];
    sc_signal<bool> sig_tb_ready_out[8];

    sc_signal<Flit> sig_tb_out[8];
    sc_signal<bool> sig_tb_valid_out[8];
    sc_signal<bool> sig_tb_ready_in[8];

    // ----------------------------------------------------------
    // 4. Conectar clock e reset
    // ----------------------------------------------------------
    noc.clk(clk);
    noc.rst(rst);
    tb.clk(clk);
    tb.rst(rst);

    // ----------------------------------------------------------
    // 5. Conectar portas de tráfego entre TB e NoC
    // ----------------------------------------------------------
    for (int i = 0; i < 8; i++) {
        // TB -> NoC (injeção de flits pelo testbench)
        tb.tb_in[i](sig_tb_in[i]);
        tb.tb_valid_in[i](sig_tb_valid_in[i]);
        tb.tb_ready_out[i](sig_tb_ready_out[i]);

        noc.tb_in[i](sig_tb_in[i]);
        noc.tb_valid_in[i](sig_tb_valid_in[i]);
        noc.tb_ready_out[i](sig_tb_ready_out[i]);

        // NoC -> TB (recepção de flits pelo monitor)
        noc.tb_out[i](sig_tb_out[i]);
        noc.tb_valid_out[i](sig_tb_valid_out[i]);
        noc.tb_ready_in[i](sig_tb_ready_in[i]);

        tb.tb_out[i](sig_tb_out[i]);
        tb.tb_valid_out[i](sig_tb_valid_out[i]);
        tb.tb_ready_in[i](sig_tb_ready_in[i]);
    }

    // ----------------------------------------------------------
    // 6. Configurar rastreio VCD
    // ----------------------------------------------------------
    sc_trace_file* tf = sc_create_vcd_trace_file("simulation_waveform");
    tf->set_time_unit(1, SC_NS);

    // Sinais globais
    sc_trace(tf, clk, "clk");
    sc_trace(tf, rst, "rst");

    // Interface HOST 0 (origem do pior caso e do melhor caso)
    sc_trace(tf, sig_tb_in[0],        "HOST0_flit_in");
    sc_trace(tf, sig_tb_valid_in[0],  "HOST0_valid_in");
    sc_trace(tf, sig_tb_ready_out[0], "HOST0_ready_out");

    // Interface HOST 7 (destino do pior caso)
    sc_trace(tf, sig_tb_out[7],       "HOST7_flit_out");
    sc_trace(tf, sig_tb_valid_out[7], "HOST7_valid_out");
    sc_trace(tf, sig_tb_ready_in[7],  "HOST7_ready_in");

    // Interface HOST 1 (destino do melhor caso)
    sc_trace(tf, sig_tb_out[1],       "HOST1_flit_out");
    sc_trace(tf, sig_tb_valid_out[1], "HOST1_valid_out");
    sc_trace(tf, sig_tb_ready_in[1],  "HOST1_ready_in");

    // Interface HOST 2 (origem do cenário 2)
    sc_trace(tf, sig_tb_in[2],        "HOST2_flit_in");
    sc_trace(tf, sig_tb_valid_in[2],  "HOST2_valid_in");
    sc_trace(tf, sig_tb_ready_out[2], "HOST2_ready_out");

    // Interface HOST 5 (destino do cenário 2)
    sc_trace(tf, sig_tb_out[5],       "HOST5_flit_out");
    sc_trace(tf, sig_tb_valid_out[5], "HOST5_valid_out");
    sc_trace(tf, sig_tb_ready_in[5],  "HOST5_ready_in");

    // ----------------------------------------------------------
    // 7. Executar a simulação
    // ----------------------------------------------------------
    std::cout << "[SIM] Iniciando simulação da NoC SPIN..." << std::endl;
    std::cout << "[SIM] Duração total: 2000 ns (200 ciclos de clock)" << std::endl;
    std::cout << "[SIM] Período do clock: 10 ns" << std::endl;
    std::cout << "[SIM] Reset ativo nos primeiros 3 ciclos (30 ns)" << std::endl;
    std::cout << std::endl;

    sc_start(2000, SC_NS);

    // ----------------------------------------------------------
    // 8. Finalizar e fechar arquivo VCD
    // ----------------------------------------------------------
    sc_close_vcd_trace_file(tf);

    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "[SIM] Simulação finalizada com sucesso!" << std::endl;
    std::cout << "[SIM] Arquivo de ondas gerado: simulation_waveform.vcd" << std::endl;
    std::cout << "[SIM] Visualize com: gtkwave simulation_waveform.vcd" << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}