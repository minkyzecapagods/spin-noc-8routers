#ifndef ROUTER_H
#define ROUTER_H

#include <systemc.h>
#include <cstdio>
#include "noc_pkg.h"
#include "fifo_buffer.h"
#include "routing_logic.h"
#include "arbiter.h"
#include "crossbar.h"

// ============================================================
// Módulo: Router
// Descrição: Roteador completo para a NoC SPIN (Fat-Tree).
//            Cada roteador possui NUM_TOTAL_PORTAS (6) portas
//            bidireccionais. Internamente instancia:
//              - 1 FifoBuffer por porta de entrada (6 FIFOs)
//              - 1 RoutingLogic por FIFO (6 lógicas de rota)
//              - 1 Arbiter por porta de saída (6 árbitros)
//              - 1 Crossbar central
//
// Interface externa (por porta):
//   flit_in[i]   / flit_out[i]   : dados
//   valid_in[i]  / valid_out[i]  : handshake
//   ready_in[i]  / ready_out[i]  : backpressure
// ============================================================
SC_MODULE(Router) {
    // ----- Parâmetro de configuração -----
    int router_id;

    // ----- Sinais de controle globais -----
    sc_in<bool> clk;
    sc_in<bool> rst;

    // ----- Portas externas de entrada (uma por porta física) -----
    sc_in<Flit>  flit_in[NUM_TOTAL_PORTAS];
    sc_in<bool>  valid_in[NUM_TOTAL_PORTAS];
    sc_out<bool> ready_in[NUM_TOTAL_PORTAS]; // Backpressure para o transmissor

    // ----- Portas externas de saída (uma por porta física) -----
    sc_out<Flit> flit_out[NUM_TOTAL_PORTAS];
    sc_out<bool> valid_out[NUM_TOTAL_PORTAS];
    sc_in<bool>  ready_out[NUM_TOTAL_PORTAS]; // Backpressure do receptor

    // =========================================================
    // Submódulos internos
    // =========================================================
    FifoBuffer*   fifos[NUM_TOTAL_PORTAS];
    RoutingLogic* rotas[NUM_TOTAL_PORTAS];
    Arbiter*      arbitros[NUM_TOTAL_PORTAS]; // Um por porta de SAÍDA
    Crossbar*     crossbar;

    // =========================================================
    // Sinais internos de interconexão
    // =========================================================

    // FIFO -> RoutingLogic
    sc_signal<Flit> sig_fifo_dout[NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_fifo_valid[NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_fifo_ready[NUM_TOTAL_PORTAS]; // Crossbar -> FIFO (pop)

    // RoutingLogic -> Arbiter
    // req_saida[entrada][saida]: porta 'entrada' quer usar porta 'saida'
    sc_signal<int>  sig_porta_saida[NUM_TOTAL_PORTAS]; // Saída escolhida pela lógica
    sc_signal<bool> sig_req_valida[NUM_TOTAL_PORTAS];  // Há requisição?

    // Requisições remapeadas: [porta_saida][porta_entrada]
    sc_signal<bool> sig_req[NUM_TOTAL_PORTAS][NUM_TOTAL_PORTAS];

    // Arbiter -> Crossbar
    sc_signal<int>  sig_grant[NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_grant_ok[NUM_TOTAL_PORTAS];

    // Crossbar -> portas de saída (e ready de volta)
    sc_signal<Flit> sig_xbar_out[NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_xbar_valid[NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_xbar_ready_in[NUM_TOTAL_PORTAS];

    // =========================================================
    // Construtor parametrizado
    // =========================================================
    Router(sc_module_name name, int id) : sc_module(name), router_id(id) {
        // --------------------------------------------------
        // 1. Instancia e conecta as FIFOs de entrada
        // --------------------------------------------------
        for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
            char nome[32];
            std::sprintf(nome, "FIFO_R%d_P%d", router_id, i);
            fifos[i] = new FifoBuffer(nome);

            fifos[i]->clk(clk);
            fifos[i]->rst(rst);

            // Entrada externa -> FIFO
            fifos[i]->din(flit_in[i]);
            fifos[i]->din_valid(valid_in[i]);
            fifos[i]->din_ready(ready_in[i]);

            // FIFO -> sinais internos para lógica de rota
            fifos[i]->dout(sig_fifo_dout[i]);
            fifos[i]->dout_valid(sig_fifo_valid[i]);
            fifos[i]->dout_ready(sig_fifo_ready[i]); // Crossbar confirma leitura
        }

        // --------------------------------------------------
        // 2. Instancia e conecta as lógicas de roteamento
        // --------------------------------------------------
        for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
            char nome[32];
            std::sprintf(nome, "Rota_R%d_P%d", router_id, i);
            rotas[i] = new RoutingLogic(nome, router_id);

            rotas[i]->flit_in(sig_fifo_dout[i]);
            rotas[i]->flit_valid(sig_fifo_valid[i]);
            rotas[i]->porta_saida(sig_porta_saida[i]);
            rotas[i]->req_valida(sig_req_valida[i]);
        }

        // --------------------------------------------------
        // 3. Lógica de remapeamento: converte
        //    sig_porta_saida[entrada] -> sig_req[saida][entrada]
        //    Isso é feito por um SC_METHOD dedicado
        // --------------------------------------------------
        SC_METHOD(remapeia_requisicoes);
        for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
            sensitive << sig_porta_saida[i] << sig_req_valida[i];
        }

        // --------------------------------------------------
        // 4. Instancia e conecta os árbitros (um por saída)
        // --------------------------------------------------
        for (int j = 0; j < NUM_TOTAL_PORTAS; j++) {
            char nome[32];
            std::sprintf(nome, "Arb_R%d_S%d", router_id, j);
            arbitros[j] = new Arbiter(nome);

            arbitros[j]->clk(clk);
            arbitros[j]->rst(rst);

            // Requisições das entradas para esta saída j
            for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
                arbitros[j]->req_in[i](sig_req[j][i]);
            }

            arbitros[j]->grant_out(sig_grant[j]);
            arbitros[j]->grant_valido(sig_grant_ok[j]);
        }

        // --------------------------------------------------
        // 5. Instancia e conecta a crossbar
        // --------------------------------------------------
        crossbar = new Crossbar("Xbar");

        for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
            crossbar->flit_in[i](sig_fifo_dout[i]);
            crossbar->valid_in[i](sig_fifo_valid[i]);
            crossbar->grant[i](sig_grant[i]);
            crossbar->grant_ok[i](sig_grant_ok[i]);
            crossbar->ready_out[i](ready_out[i]); // Backpressure externo
            crossbar->flit_out[i](sig_xbar_out[i]);
            crossbar->valid_out[i](sig_xbar_valid[i]);
            crossbar->ready_in[i](sig_xbar_ready_in[i]);
        }

        // --------------------------------------------------
        // 6. Conecta saídas da crossbar às portas externas
        //    e propaga backpressure de volta às FIFOs
        // --------------------------------------------------
        SC_METHOD(conecta_saidas);
        for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
            sensitive << sig_xbar_out[i] << sig_xbar_valid[i]
                      << sig_xbar_ready_in[i];
        }
    }

    // =========================================================
    // Métodos auxiliares
    // =========================================================

    // Converte: para cada entrada i com sig_porta_saida[i]=j,
    // ativa sig_req[j][i] = sig_req_valida[i]
    void remapeia_requisicoes() {
        // Zera todas as requisições
        for (int j = 0; j < NUM_TOTAL_PORTAS; j++)
            for (int i = 0; i < NUM_TOTAL_PORTAS; i++)
                sig_req[j][i].write(false);

        // Ativa apenas as requisições válidas
        for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
            if (sig_req_valida[i].read()) {
                int j = sig_porta_saida[i].read();
                if (j >= 0 && j < NUM_TOTAL_PORTAS) {
                    sig_req[j][i].write(true);
                }
            }
        }
    }

    // Propaga saídas da crossbar para portas externas e
    // backpressure de retorno para as FIFOs
    void conecta_saidas() {
        for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
            flit_out[i].write(sig_xbar_out[i].read());
            valid_out[i].write(sig_xbar_valid[i].read());
            sig_fifo_ready[i].write(sig_xbar_ready_in[i].read());
        }
    }

    // Destrutor: libera submódulos
    ~Router() {
        for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
            delete fifos[i];
            delete rotas[i];
            delete arbitros[i];
        }
        delete crossbar;
    }
};

#endif // ROUTER_H
