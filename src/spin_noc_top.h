#ifndef SPIN_NOC_TOP_H
#define SPIN_NOC_TOP_H

#include <systemc.h>
#include <cstdio>
#include "router.h"
#include "noc_pkg.h"

struct LinkSinais {
    sc_signal<Flit> data;
    sc_signal<bool> valid;
    sc_signal<bool> ready;
};

struct LinkBidirecional {
    LinkSinais ida;   // r_a -> r_b
    LinkSinais volta; // r_b -> r_a
};

SC_MODULE(SpinNocTop) {
    // ----- Sinais de controle globais -----
    sc_in<bool> clk;
    sc_in<bool> rst;

    // ----- Interface com o Testbench (8 hosts) -----
    sc_in<Flit>  tb_in[8];
    sc_in<bool>  tb_valid_in[8];
    sc_out<bool> tb_ready_out[8];

    sc_out<Flit> tb_out[8];
    sc_out<bool> tb_valid_out[8];
    sc_in<bool>  tb_ready_in[8];

    // ----- 8 Roteadores -----
    Router* routers[8];

    // =========================================================
    // Links internos da topologia Fat-Tree (full-duplex)
    // =========================================================

    // Root 4 connects Leaf 0 (Cluster A) <-> Leaf 2 (Cluster B)
    LinkBidirecional link_R0_UP0_R4_DOWN0;
    LinkBidirecional link_R2_UP0_R4_DOWN1; 

    // Root 5 connects Leaf 0 (Cluster A) <-> Leaf 3 (Cluster B)
    LinkBidirecional link_R0_UP1_R5_DOWN0;
    LinkBidirecional link_R3_UP0_R5_DOWN1; 

    // Root 6 connects Leaf 1 (Cluster A) <-> Leaf 2 (Cluster B)
    LinkBidirecional link_R1_UP0_R6_DOWN0; 
    LinkBidirecional link_R2_UP1_R6_DOWN1; 

    // Root 7 connects Leaf 1 (Cluster A) <-> Leaf 3 (Cluster B)
    LinkBidirecional link_R1_UP1_R7_DOWN0; 
    LinkBidirecional link_R3_UP1_R7_DOWN1;

    SC_CTOR(SpinNocTop) {
        for (int i = 0; i < 8; i++) {
            char nome[32];
            std::sprintf(nome, "Router_%d", i);
            routers[i] = new Router(nome, i);
            routers[i]->clk(clk);
            routers[i]->rst(rst);
        }

        for (int host = 0; host < 8; host++) {
            int r     = host / 2;         
            int porta = host % 2;         

            routers[r]->flit_in[porta](tb_in[host]);
            routers[r]->valid_in[porta](tb_valid_in[host]);
            routers[r]->ready_in[porta](tb_ready_out[host]);

            routers[r]->flit_out[porta](tb_out[host]);
            routers[r]->valid_out[porta](tb_valid_out[host]);
            routers[r]->ready_out[porta](tb_ready_in[host]);
        }

        // --- Router 0 UP_0 <-> Router 4 DOWN_0 ---
        conecta_link(routers[0], UP_0, routers[4], DOWN_0, link_R0_UP0_R4_DOWN0);
        // --- Router 2 UP_0 <-> Router 4 DOWN_1 ---
        conecta_link(routers[2], UP_0, routers[4], DOWN_1, link_R2_UP0_R4_DOWN1);

        // --- Router 0 UP_1 <-> Router 5 DOWN_0 ---
        conecta_link(routers[0], UP_1, routers[5], DOWN_0, link_R0_UP1_R5_DOWN0);
        // --- Router 3 UP_0 <-> Router 5 DOWN_1 ---
        conecta_link(routers[3], UP_0, routers[5], DOWN_1, link_R3_UP0_R5_DOWN1);

        // --- Router 1 UP_0 <-> Router 6 DOWN_0 ---
        conecta_link(routers[1], UP_0, routers[6], DOWN_0, link_R1_UP0_R6_DOWN0);
        // --- Router 2 UP_1 <-> Router 6 DOWN_1 ---
        conecta_link(routers[2], UP_1, routers[6], DOWN_1, link_R2_UP1_R6_DOWN1);

        // --- Router 1 UP_1 <-> Router 7 DOWN_0 ---
        conecta_link(routers[1], UP_1, routers[7], DOWN_0, link_R1_UP1_R7_DOWN0);
        // --- Router 3 UP_1 <-> Router 7 DOWN_1 ---
        conecta_link(routers[3], UP_1, routers[7], DOWN_1, link_R3_UP1_R7_DOWN1);


        for (int r = 0; r < 8; r++) {
            bool eh_folha = (r < 4);
            int portas_nao_usadas[4];
            int qtd_portas;

            if (eh_folha) {
                portas_nao_usadas[0] = (int)DOWN_0;
                portas_nao_usadas[1] = (int)DOWN_1;
                qtd_portas = 2;
            } else {
                portas_nao_usadas[0] = (int)LOCAL_0;
                portas_nao_usadas[1] = (int)LOCAL_1;
                portas_nao_usadas[2] = (int)UP_0;
                portas_nao_usadas[3] = (int)UP_1;
                qtd_portas = 4;
            }

            for (int k = 0; k < qtd_portas; k++) {
                int p = portas_nao_usadas[k];

                routers[r]->flit_in[p](sig_dummy_data[r][p]);
                routers[r]->valid_in[p](sig_dummy_valid[r][p]);
                routers[r]->ready_in[p](sig_dummy_ready_in[r][p]);

                routers[r]->flit_out[p](sig_dummy_data_out[r][p]);
                routers[r]->valid_out[p](sig_dummy_valid_out[r][p]);
                routers[r]->ready_out[p](sig_dummy_ready[r][p]);
            }
        }
    }

    void conecta_link(Router* r_a, int porta_a,
                      Router* r_b, int porta_b,
                      LinkBidirecional& link)
    {
        // ---- Sentido A -> B (ida) ----
        r_a->flit_out[porta_a](link.ida.data);
        r_a->valid_out[porta_a](link.ida.valid);
        r_a->ready_out[porta_a](link.ida.ready);

        r_b->flit_in[porta_b](link.ida.data);
        r_b->valid_in[porta_b](link.ida.valid);
        r_b->ready_in[porta_b](link.ida.ready);

        // ---- Sentido B -> A (volta) ----
        r_b->flit_out[porta_b](link.volta.data);
        r_b->valid_out[porta_b](link.volta.valid);
        r_b->ready_out[porta_b](link.volta.ready);

        r_a->flit_in[porta_a](link.volta.data);
        r_a->valid_in[porta_a](link.volta.valid);
        r_a->ready_in[porta_a](link.volta.ready);
    }

    sc_signal<Flit> sig_dummy_data[8][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_valid[8][NUM_TOTAL_PORTAS];
    sc_signal<Flit> sig_dummy_data_out[8][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_valid_out[8][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_ready[8][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_ready_in[8][NUM_TOTAL_PORTAS];

    ~SpinNocTop() {
        for (int i = 0; i < 8; i++) delete routers[i];
    }
};

#endif // SPIN_NOC_TOP_H
