#ifndef ARBITER_H
#define ARBITER_H

#include <systemc.h>

// TODO: ÁRBITRO ROUND-ROBIN
// 1. Receber requisições de rota das diversas entradas que disputam a mesma saída.
// 2. Implementar um contador/ponteiro circular para definir quem ganha a vez (Grant), evitando Starvation.

SC_MODULE(Arbiter) {
    SC_CTOR(Arbiter) {
        // TODO: Lógica do árbitro
    }
};

#endif
