#ifndef SPIN_NOC_TOP_H
#define SPIN_NOC_TOP_H

#include <systemc.h>
#include "router.h"

// TODO: TOP-LEVEL DA REDE (CONEXÃO DOS 8 ROTEADORES)
// 1. Instanciar um array de 8 objetos do tipo 'Router'.
// 2. Criar uma malha de 'sc_signal<Flit>' para simular os canais físicos entre eles.
// 3. Fazer a fiação manual simulando a árvore SPIN (Fat-Tree).
// 4. Deixar expostas as 8 portas 'LOCAL' para que o Testbench possa injetar dados.

SC_MODULE(SpinNocTop) {
    sc_in<bool> clk;
    sc_in<bool> rst;

    SC_CTOR(SpinNocTop) {
        // TODO: Fazer o wire-up dos 8 roteadores
    }
};

#endif
