#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <systemc.h>
#include <queue>
#include "noc_pkg.h"

// ============================================================
// Módulo: FifoBuffer
// Descrição: Buffer de entrada por porta, capacidade fixa de
//            CAPACIDADE_FIFO flits. Usa handshake valid/ready.
//
// Porta de entrada  (produtor -> FIFO):
//   din       : dado do flit a ser escrito
//   din_valid : produtor indica que din é válido
//   din_ready : FIFO sinaliza que tem espaço (aceita dado)
//
// Porta de saída (FIFO -> consumidor):
//   dout       : flit na cabeça da fila
//   dout_valid : FIFO indica que há dado disponível
//   dout_ready : consumidor sinaliza que leu/consumiu o dado
// ============================================================

SC_MODULE(FifoBuffer) {
    // ----- Sinais de controle globais -----
    sc_in<bool>  clk;
    sc_in<bool>  rst;

    // ----- Interface com o produtor (escrita na FIFO) -----
    sc_in<Flit>  din;
    sc_in<bool>  din_valid;
    sc_out<bool> din_ready;  // Pulsa 'true' quando há espaço

    // ----- Interface com o consumidor (leitura da FIFO) -----
    sc_out<Flit> dout;
    sc_out<bool> dout_valid; // 'true' enquanto houver dados
    sc_in<bool>  dout_ready; // Consumidor confirma leitura

    // ----- Estado interno -----
    std::queue<Flit> fila;

    SC_CTOR(FifoBuffer) {
        // Usa apenas um processo síncrono para garantir determinismo
        // e eliminar race conditions de delta-cycles
        SC_METHOD(processo_fifo);
        sensitive << clk.pos();
        dont_initialize();
    }

    // Processo unificado de controle da FIFO
    void processo_fifo();
};

#endif // FIFO_BUFFER_H
