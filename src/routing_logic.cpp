#include "routing_logic.h"
#include <iostream>

// ============================================================
// calcula_rota
// Implementa o algoritmo SPIN de forma puramente algorítmica,
// sem arrays ou condicionais baseados em IDs hardcoded.
// ============================================================
void RoutingLogic::calcula_rota() {
    if (!flit_valid.read()) {
        req_valida.write(false);
        porta_saida.write(LOCAL_0); // Valor padrão (inerte)
        return;
    }

    Flit f = flit_in.read();
    int  dest = f.dest_id;
    int  porta_escolhida = LOCAL_0;
    int  dest_leaf = dest / 2; // Identifica em qual folha o destino reside

    // ----------------------------------------------------------
    // ROTEADORES FOLHA (IDs 0 a 3)
    // ----------------------------------------------------------
    if (router_id < 4) {
        if (dest_leaf == router_id) {
            // Destino local: Par vai para LOCAL_0, Ímpar para LOCAL_1
            porta_escolhida = (dest % 2 == 0) ? LOCAL_0 : LOCAL_1;
        } else {
            // Roteamento de Subida
            // A topologia Fat-Tree garante que a porta UP_0 sempre leva a uma raiz
            // que alcança as folhas PARES (0 e 2). A porta UP_1 leva às ÍMPARES (1 e 3).
            porta_escolhida = (dest_leaf % 2 == 0) ? UP_0 : UP_1;
        }
    }
    // ----------------------------------------------------------
    // ROTEADORES RAIZ (IDs 4 a 7)
    // ----------------------------------------------------------
    else {
        // Roteamento de Descida
        // A topologia garante que todas as portas DOWN_0 das raízes apontam
        // para a metade esquerda (Folhas 0 e 1). As portas DOWN_1 apontam 
        // para a metade direita (Folhas 2 e 3).
        porta_escolhida = (dest_leaf < 2) ? DOWN_0 : DOWN_1;
    }

    porta_saida.write(porta_escolhida);
    req_valida.write(true);
}
