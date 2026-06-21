#include <systemc.h>
#include "../src/spin_noc_top.h"
#include "spin_noc_tb.h"

int sc_main(int argc, char* argv[]) {
    // 1. Criar sinais de Relógio (sc_clock) e Reset
    sc_clock clk("clk", 10, SC_NS); // Período de 10ns
    sc_signal<bool> rst;

    // 2. Instanciar o Top-Level da NoC e o Testbench
    SpinNocTop noc("SPIN_NoC_8_Routers");
    SpinNocTb tb("Testbench");

    // TODO: Conectar o Clock, Reset e as portas Locais entre a NoC e o TB.

    // 3. Configurar arquivo de ondas VCD para gerar os resultados do relatório
    sc_trace_file *tf = sc_create_vcd_trace_file("simulation_waveform");
    // sc_trace(tf, clk, "clk"); // Exemplo de rastreio

    // 4. Rodar a simulação
    cout << "Iniciando simulacao da NoC SPIN..." << endl;
    sc_start(2000, SC_NS); // Simula por 2000ns

    sc_close_vcd_trace_file(tf);
    cout << "Simulacao finalizada com sucesso!" << endl;
    return 0;
}
