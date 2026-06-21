#ifndef CROSSBAR_H
#define CROSSBAR_H

#include <systemc.h>
#include "noc_pkg.h"

// TODO: MATRIZ DE COMUTAÇÃO (CROSSBAR)
// 1. Mapear as conexões físicas de dados baseando-se estritamente nas decisões dos árbitros.
// 2. Fazer o chaveamento multiplexado dos structs de Flit das entradas para as saídas.

SC_MODULE(Crossbar) {
    SC_CTOR(Crossbar) {
        // TODO: Conexões internas
    }
};

#endif
