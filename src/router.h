#ifndef ROUTER_H
#define ROUTER_H

#include <systemc.h>
#include "fifo_buffer.h"
#include "routing_logic.h"
#include "arbiter.h"
#include "crossbar.h"

// TODO: INSTANCIAÇÃO INTERNA DO ROTEADOR
// 1. Instanciar as instâncias de FifoBuffer para cada porta de entrada.
// 2. Instanciar a lógica de roteamento, os árbitros e a crossbar.
// 3. Declarar sc_signals internos para interconectar esses submódulos.

SC_MODULE(Router) {
    sc_in<bool> clk;
    sc_in<bool> rst;
    // TODO: Adicionar portas externas do roteador (Canais de entrada/saída)

    SC_CTOR(Router) {
        // TODO: Interconectar submódulos via herança estrutural ou ponteiros
    }
};

#endif
