#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <systemc.h>
#include "noc_pkg.h"

// TODO: BUFFER DE ENTRADA (FIFO)
// 1. Implementar um array ou std::queue de tamanho fixo para armazenar os Flits.
// 2. Criar portas de interface (sc_in/sc_out) usando preferencialmente sinais de handshake (bool) ou sc_fifo_in/out.
// 3. Controlar flags de cheio (full) e vazio (empty).

SC_MODULE(FifoBuffer) {
    sc_in<bool> clk;
    sc_in<bool> rst;
    
    SC_CTOR(FifoBuffer) {
        // TODO: Registrar métodos/threads sensíveis ao clock
    }
};

#endif
