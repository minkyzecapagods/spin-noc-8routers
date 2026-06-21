#ifndef ROUTING_LOGIC_H
#define ROUTING_LOGIC_H

#include <systemc.h>
#include "noc_pkg.h"

// TODO: LÓGICA DE ROTEAMENTO SPIN (FAT-TREE)
// 1. Ler o dest_id do Flit que está na frente da FIFO de entrada.
// 2. Se o destino estiver abaixo na árvore, direcionar para a porta DOWN correta.
// 3. Se estiver fora da subárvore, enviar para cima (UP0 ou UP1) alternando (Round-Robin) para espalhar a carga.
// 4. Emitir uma requisição de rota para o árbitro da porta escolhida.

SC_MODULE(RoutingLogic) {
    SC_CTOR(RoutingLogic) {
        // TODO: Lógica combinacional para cálculo de rota
    }
};

#endif
