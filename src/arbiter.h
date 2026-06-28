#ifndef ARBITER_H
#define ARBITER_H

#include <systemc.h>
#include "noc_pkg.h"

// ============================================================
// Módulo: Arbiter
// Descrição: Árbitro Round-Robin com ponteiro circular.
//            Gerencia o acesso de NUM_TOTAL_PORTAS entradas
//            competindo por uma única saída, garantindo
//            ausência de starvation.
//
// Funcionamento por ciclo de clock:
//   1. Verifica quais portas têm req_in[i]=1
//   2. A partir do ponteiro 'prioridade', varre circularmente
//      até encontrar a primeira requisição ativa
//   3. Emite grant_out para a vencedora
//   4. Avança ponteiro: prioridade = (ganhador + 1) % N
//
// Entradas:
//   req_in[N]  : vetor de requisições das portas de entrada
//
// Saídas:
//   grant_out  : índice da porta que ganhou o acesso (-1 se nenhuma)
//   grant_valido : indica que grant_out é válido neste ciclo
// ============================================================
SC_MODULE(Arbiter) {
    // ----- Sinais de controle globais -----
    sc_in<bool> clk;
    sc_in<bool> rst;

    // ----- Entradas: requisições das portas -----
    sc_in<bool> req_in[NUM_TOTAL_PORTAS];

    // ----- Saídas: resultado da arbitragem -----
    sc_out<int>  grant_out;    // Índice da porta vencedora
    sc_out<bool> grant_valido; // Indica arbitragem válida neste ciclo

    // ----- Estado interno -----
    int prioridade; // Ponteiro circular do Round-Robin

    SC_CTOR(Arbiter) : prioridade(0) {
        SC_METHOD(arbitra);
        sensitive << clk.pos() << rst;
        dont_initialize();
    }

    // Método principal de arbitragem
    void arbitra();
};

#endif // ARBITER_H
