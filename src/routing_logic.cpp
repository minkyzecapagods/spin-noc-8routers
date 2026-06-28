#include "routing_logic.h"
#include <iostream>

// ============================================================
// calcula_rota
// Implementa o algoritmo SPIN para Fat-Tree de 2 níveis.
// Chamado toda vez que flit_in ou flit_valid mudam.
// ============================================================
void RoutingLogic::calcula_rota() {
    if (!flit_valid.read()) {
        // Nenhum flit válido: desativa requisição
        req_valida.write(false);
        porta_saida.write(LOCAL_0); // Valor padrão (inerte)
        return;
    }

    Flit f = flit_in.read();
    int  dest = f.dest_id;
    int  porta_escolhida = LOCAL_0;

    // ----------------------------------------------------------
    // ROTEADORES FOLHA (IDs 0 a 3)
    // Cada folha atende 2 hosts locais e pode subir para 2 raízes
    // ----------------------------------------------------------
    if (router_id >= 0 && router_id <= 3) {
        if ((dest / 2) == router_id) {
            // Destino está conectado diretamente a este roteador
            if (dest % 2 == 0) {
                porta_escolhida = LOCAL_0; // Host par  (ex: Host 0, 2, 4, 6)
            } else {
                porta_escolhida = LOCAL_1; // Host ímpar (ex: Host 1, 3, 5, 7)
            }
            // std::cout << "@" << sc_time_stamp()
            //           << " [Roteamento R" << router_id
            //           << "] Destino local! Flit dest=" << dest
            //           << " -> Porta " << porta_escolhida << std::endl;
        } else {
            // Destino fora desta subárvore: subir pela árvore
            // Alterna entre UP_0 e UP_1 para balanceamento de carga
            if (!toggle_up) {
                porta_escolhida = UP_0;
            } else {
                porta_escolhida = UP_1;
            }
            toggle_up = !toggle_up; // Inverte o toggle para o próximo pacote

            // std::cout << "@" << sc_time_stamp()
            //           << " [Roteamento R" << router_id
            //           << "] Subindo! Flit dest=" << dest
            //           << " -> Porta " << (porta_escolhida == UP_0 ? "UP_0" : "UP_1")
            //           << std::endl;
        }
    }
    // ----------------------------------------------------------
    // ROTEADORES RAIZ (IDs 4 a 7)
    // Só encaminham para baixo (DOWN), nunca para cima
    // DOWN_0: subárvore com hosts 0-3
    // DOWN_1: subárvore com hosts 4-7
    // ----------------------------------------------------------
    else if (router_id >= 4 && router_id <= 7) {
        if (dest >= 0 && dest <= 3) {
            porta_escolhida = DOWN_0; // Subárvore esquerda
        } else {
            porta_escolhida = DOWN_1; // Subárvore direita
        }

        // std::cout << "@" << sc_time_stamp()
        //           << " [Roteamento R" << router_id
        //           << "] Descendo! Flit dest=" << dest
        //           << " -> Porta " << (porta_escolhida == DOWN_0 ? "DOWN_0" : "DOWN_1")
        //           << std::endl;
    }

    porta_saida.write(porta_escolhida);
    req_valida.write(true);
}
