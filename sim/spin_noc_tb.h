#ifndef SPIN_NOC_TB_H
#define SPIN_NOC_TB_H

#include <systemc.h>
#include "../src/noc_pkg.h"

// TODO: GERADOR DE TRÁFEGO (TESTBENCH)
// 1. Criar threads (SC_THREAD) independentes para injetar pacotes nas portas LOCAL de forma concorrente.
// 2. Validar cenários de teste: Ex: Roteador 0 enviando para o Roteador 7 (pior caso de saltos na árvore).
// 3. Verificar integridade (se os dados chegaram corretos) e medir latência (ciclos decorridos).

SC_MODULE(SpinNocTb) {
    sc_in<bool> clk;
    sc_out<bool> rst;

    SC_THREAD(traffic_generator);

    SC_CTOR(SpinNocTb) {
    }
    
    void traffic_generator();
};

#endif
