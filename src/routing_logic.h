#ifndef ROUTING_LOGIC_H
#define ROUTING_LOGIC_H

#include <systemc.h>
#include "noc_pkg.h"

// ============================================================
// Módulo: RoutingLogic
// Descrição: Calcula a porta de saída para um flit recebido,
//            seguindo o algoritmo SPIN em árvore Fat-Tree de
//            2 níveis com 8 hosts (4 roteadores folha + 4 raiz).
//
// Regras de roteamento:
//   Folhas (router_id 0-3):
//     - Se (dest_id / 2) == router_id → destino é local
//       * dest_id par  → LOCAL_0
//       * dest_id ímpar → LOCAL_1
//     - Caso contrário → alterna entre UP_0 e UP_1 (toggle)
//
//   Raízes (router_id 4-7):
//     - dest_id 0-3 → DOWN_0 (subárvore esquerda)
//     - dest_id 4-7 → DOWN_1 (subárvore direita)
//
// Entradas:
//   flit_in      : flit a ser roteado
//   flit_valid   : indica que flit_in é válido
//
// Saídas:
//   porta_saida  : índice da porta destino (PortID)
//   req_valida   : indica que há uma requisição para o árbitro
// ============================================================
SC_MODULE(RoutingLogic) {
    // ----- Parâmetro de configuração -----
    int router_id; // ID deste roteador (0 a 7)

    // ----- Entradas -----
    sc_in<Flit> flit_in;
    sc_in<bool> flit_valid;

    // ----- Saídas -----
    sc_out<int>  porta_saida; // Porta escolhida pelo algoritmo SPIN
    sc_out<bool> req_valida;  // Requisição ativa para o árbitro

    // ----- Estado interno -----
    bool toggle_up; // Alterna entre UP_0 e UP_1 (balanceamento de carga)

    // Construtor parametrizado
    RoutingLogic(sc_module_name name, int id)
        : sc_module(name), router_id(id), toggle_up(false) {
        // Necessário porque este construtor é parametrizado (não usa
        // SC_CTOR), e o SC_METHOD abaixo precisa de SC_CURRENT_USER_MODULE.
        SC_HAS_PROCESS(RoutingLogic);

        SC_METHOD(calcula_rota);
        sensitive << flit_in << flit_valid;
    }

    // Método principal de cálculo de rota
    void calcula_rota();
};

#endif // ROUTING_LOGIC_H
