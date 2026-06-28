#ifndef CROSSBAR_H
#define CROSSBAR_H

#include <systemc.h>
#include "noc_pkg.h"

// ============================================================
// Módulo: Crossbar
// Descrição: Matriz de comutação NUM_TOTAL_PORTAS x NUM_TOTAL_PORTAS.
//            Baseada nos grants dos árbitros, conecta dinamicamente
//            uma porta de entrada a uma porta de saída por ciclo.
//
// Para cada porta de saída 'j':
//   - Existe um árbitro dedicado que emite grant[j] = i
//   - A crossbar copia flit_in[i] -> flit_out[j]
//   - Propaga os sinais de handshake valid/ready corretamente
//
// Entradas:
//   flit_in[N]    : dados das N FIFOs de entrada
//   valid_in[N]   : válidos das N FIFOs
//   grant[N]      : índice da porta de entrada selecionada para cada saída
//   grant_ok[N]   : indica que o grant[j] é válido neste ciclo
//
// Saídas:
//   flit_out[N]   : dados para as N portas de saída físicas
//   valid_out[N]  : válidos para as N portas de saída
//   ready_in[N]   : backpressure propagado da saída para a entrada vencedora
// ============================================================
SC_MODULE(Crossbar) {
    // ----- Entradas das FIFOs -----
    sc_in<Flit> flit_in[NUM_TOTAL_PORTAS];
    sc_in<bool> valid_in[NUM_TOTAL_PORTAS];

    // ----- Grants dos árbitros (um por porta de saída) -----
    sc_in<int>  grant[NUM_TOTAL_PORTAS];
    sc_in<bool> grant_ok[NUM_TOTAL_PORTAS];

    // ----- Backpressure das portas de saída -----
    sc_in<bool> ready_out[NUM_TOTAL_PORTAS]; // Saída externa aceita?

    // ----- Saídas físicas do roteador -----
    sc_out<Flit> flit_out[NUM_TOTAL_PORTAS];
    sc_out<bool> valid_out[NUM_TOTAL_PORTAS];

    // ----- Backpressure propagado de volta às FIFOs -----
    sc_out<bool> ready_in[NUM_TOTAL_PORTAS];

    SC_CTOR(Crossbar) {
        SC_METHOD(chaveia);
        for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
            sensitive << flit_in[i] << valid_in[i]
                      << grant[i]   << grant_ok[i]
                      << ready_out[i];
        }
    }

    // Método principal de chaveamento
    void chaveia();
};

#endif // CROSSBAR_H
