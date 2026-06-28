#ifndef SPIN_NOC_TOP_H
#define SPIN_NOC_TOP_H

#include <systemc.h>
#include <cstdio>
#include "router.h"
#include "noc_pkg.h"

// ============================================================
// Módulo: SpinNocTop
// Descrição: Top-level da NoC SPIN com 8 roteadores formando
//            uma Fat-Tree de 2 níveis:
//
//            Nível Folha (Leaf) — Roteadores 0 a 3:
//              Router 0: HOST 0 (LOCAL_0) e HOST 1 (LOCAL_1)
//              Router 1: HOST 2 (LOCAL_0) e HOST 3 (LOCAL_1)
//              Router 2: HOST 4 (LOCAL_0) e HOST 5 (LOCAL_1)
//              Router 3: HOST 6 (LOCAL_0) e HOST 7 (LOCAL_1)
//
//            Nível Raiz (Root) — Roteadores 4 a 7:
//              Cada raiz conecta pares de folhas via DOWN_0/DOWN_1
//
//            Topologia de interconexão UP/DOWN:
//              Router 0 UP_0 <-> Router 4 DOWN_0
//              Router 0 UP_1 <-> Router 5 DOWN_0
//              Router 1 UP_0 <-> Router 4 DOWN_1
//              Router 1 UP_1 <-> Router 5 DOWN_1
//              Router 2 UP_0 <-> Router 6 DOWN_0
//              Router 2 UP_1 <-> Router 7 DOWN_0
//              Router 3 UP_0 <-> Router 6 DOWN_1
//              Router 3 UP_1 <-> Router 7 DOWN_1
// ============================================================

// ============================================================
// Estrutura auxiliar para agrupar os 3 sinais de uma conexão
// ponto-a-ponto entre duas portas de roteadores distintos
// ============================================================
struct LinkSinais {
    sc_signal<Flit> data;
    sc_signal<bool> valid;
    sc_signal<bool> ready;
};

SC_MODULE(SpinNocTop) {
    // ----- Sinais de controle globais -----
    sc_in<bool> clk;
    sc_in<bool> rst;

    // ----- Interface com o Testbench (8 hosts) -----
    // Entrada no host i -> porta LOCAL do roteador folha
    sc_in<Flit>  tb_in[8];
    sc_in<bool>  tb_valid_in[8];
    sc_out<bool> tb_ready_out[8];

    // Saída do host i <- porta LOCAL do roteador folha
    sc_out<Flit> tb_out[8];
    sc_out<bool> tb_valid_out[8];
    sc_in<bool>  tb_ready_in[8];

    // ----- 8 Roteadores -----
    Router* routers[8];

    // =========================================================
    // Links internos da topologia Fat-Tree
    // Nomenclatura: link_Ri_Pj_Rk_Pl
    //   Ri porta Pj <-> Rk porta Pl (bidirecional)
    // =========================================================

    // Links das folhas com as raízes (UP da folha <-> DOWN da raiz)
    // Router 0 <-> Router 4 e Router 5
    LinkSinais link_R0_UP0_R4_DOWN0; // Router 0 UP_0  <-> Router 4 DOWN_0
    LinkSinais link_R0_UP1_R5_DOWN0; // Router 0 UP_1  <-> Router 5 DOWN_0
    // Router 1 <-> Router 4 e Router 5
    LinkSinais link_R1_UP0_R4_DOWN1; // Router 1 UP_0  <-> Router 4 DOWN_1
    LinkSinais link_R1_UP1_R5_DOWN1; // Router 1 UP_1  <-> Router 5 DOWN_1
    // Router 2 <-> Router 6 e Router 7
    LinkSinais link_R2_UP0_R6_DOWN0; // Router 2 UP_0  <-> Router 6 DOWN_0
    LinkSinais link_R2_UP1_R7_DOWN0; // Router 2 UP_1  <-> Router 7 DOWN_0
    // Router 3 <-> Router 6 e Router 7
    LinkSinais link_R3_UP0_R6_DOWN1; // Router 3 UP_0  <-> Router 6 DOWN_1
    LinkSinais link_R3_UP1_R7_DOWN1; // Router 3 UP_1  <-> Router 7 DOWN_1

    // =========================================================
    // Construtor
    // =========================================================
    SC_CTOR(SpinNocTop) {
        // --------------------------------------------------
        // 1. Instancia os 8 roteadores
        // --------------------------------------------------
        for (int i = 0; i < 8; i++) {
            char nome[32];
            std::sprintf(nome, "Router_%d", i);
            routers[i] = new Router(nome, i);
            routers[i]->clk(clk);
            routers[i]->rst(rst);
        }

        // --------------------------------------------------
        // 2. Conecta a interface do Testbench às portas
        //    LOCAL_0 e LOCAL_1 dos roteadores folha (0-3)
        //
        //    Host 0 <-> Router 0 LOCAL_0
        //    Host 1 <-> Router 0 LOCAL_1
        //    Host 2 <-> Router 1 LOCAL_0
        //    Host 3 <-> Router 1 LOCAL_1
        //    Host 4 <-> Router 2 LOCAL_0
        //    Host 5 <-> Router 2 LOCAL_1
        //    Host 6 <-> Router 3 LOCAL_0
        //    Host 7 <-> Router 3 LOCAL_1
        // --------------------------------------------------
        for (int host = 0; host < 8; host++) {
            int r     = host / 2;         // Índice do roteador folha (0-3)
            int porta = host % 2;         // LOCAL_0 ou LOCAL_1

            // TB -> Roteador (flit entra no roteador)
            routers[r]->flit_in[porta](tb_in[host]);
            routers[r]->valid_in[porta](tb_valid_in[host]);
            routers[r]->ready_in[porta](tb_ready_out[host]);

            // Roteador -> TB (flit sai do roteador para o host)
            routers[r]->flit_out[porta](tb_out[host]);
            routers[r]->valid_out[porta](tb_valid_out[host]);
            routers[r]->ready_out[porta](tb_ready_in[host]);
        }

        // --------------------------------------------------
        // 3. Conecta os links UP/DOWN entre folhas e raízes
        //    Cada link é bidirecional: usa dois conjuntos de
        //    sinais (um para cada sentido de transmissão)
        //
        //    Convenção de nomes dos sinais internos:
        //      link_RX_UPn_RY_DOWNm.data  : dado
        //      link_RX_UPn_RY_DOWNm.valid : válido
        //      link_RX_UPn_RY_DOWNm.ready : pronto
        //    Nesta implementação simplificada, o link é
        //    tratado como simplex por sentido usando os
        //    sinais flit_out/valid_out do transmissor ligados
        //    a flit_in/valid_in do receptor.
        // --------------------------------------------------

        // --- Router 0 UP_0 <-> Router 4 DOWN_0 ---
        conecta_link(routers[0], UP_0, routers[4], DOWN_0,
                     link_R0_UP0_R4_DOWN0);

        // --- Router 0 UP_1 <-> Router 5 DOWN_0 ---
        conecta_link(routers[0], UP_1, routers[5], DOWN_0,
                     link_R0_UP1_R5_DOWN0);

        // --- Router 1 UP_0 <-> Router 4 DOWN_1 ---
        conecta_link(routers[1], UP_0, routers[4], DOWN_1,
                     link_R1_UP0_R4_DOWN1);

        // --- Router 1 UP_1 <-> Router 5 DOWN_1 ---
        conecta_link(routers[1], UP_1, routers[5], DOWN_1,
                     link_R1_UP1_R5_DOWN1);

        // --- Router 2 UP_0 <-> Router 6 DOWN_0 ---
        conecta_link(routers[2], UP_0, routers[6], DOWN_0,
                     link_R2_UP0_R6_DOWN0);

        // --- Router 2 UP_1 <-> Router 7 DOWN_0 ---
        conecta_link(routers[2], UP_1, routers[7], DOWN_0,
                     link_R2_UP1_R7_DOWN0);

        // --- Router 3 UP_0 <-> Router 6 DOWN_1 ---
        conecta_link(routers[3], UP_0, routers[6], DOWN_1,
                     link_R3_UP0_R6_DOWN1);

        // --- Router 3 UP_1 <-> Router 7 DOWN_1 ---
        conecta_link(routers[3], UP_1, routers[7], DOWN_1,
                     link_R3_UP1_R7_DOWN1);

        // --------------------------------------------------
        // 4. Conecta portas não utilizadas dos roteadores
        //    raiz (4-7) a sinais de aterramento (dummy).
        //
        //    Roteadores raiz NÃO têm hosts locais nem links
        //    UP (não há terceiro nível de árvore). As portas
        //    LOCAL_0, LOCAL_1, UP_0 e UP_1 ficam inativas.
        //    É obrigatório conectar todos os sc_in/sc_out
        //    para evitar erros de binding do SystemC.
        // --------------------------------------------------
        for (int r = 4; r <= 7; r++) {
            for (int p : {(int)LOCAL_0, (int)LOCAL_1,
                          (int)UP_0,    (int)UP_1}) {
                int idx = r - 4;
                // Entradas do roteador raiz vindas de "nenhum lugar"
                routers[r]->flit_in[p](sig_dummy_data[idx][p]);
                routers[r]->valid_in[p](sig_dummy_valid[idx][p]);
                // ready_in: roteador raiz indica que pode receber (sink dummy)
                routers[r]->ready_in[p](sig_dummy_ready_in[idx][p]);
                // Saídas do roteador raiz indo para "nenhum lugar"
                routers[r]->flit_out[p](sig_dummy_data_out[idx][p]);
                routers[r]->valid_out[p](sig_dummy_valid_out[idx][p]);
                // ready_out: raiz recebe backpressure do "nenhum lugar" (sempre false)
                routers[r]->ready_out[p](sig_dummy_ready[idx][p]);
            }
        }
    }

    // =========================================================
    // Método auxiliar: conecta porta de saída de um roteador à
    // porta de entrada de outro via sinais compartilhados
    // =========================================================
    void conecta_link(Router* r_a, int porta_a,
                      Router* r_b, int porta_b,
                      LinkSinais& link)
    {
        // Sentido A -> B: saída de r_a entra em r_b
        r_a->flit_out[porta_a](link.data);
        r_a->valid_out[porta_a](link.valid);
        r_a->ready_out[porta_a](link.ready);

        r_b->flit_in[porta_b](link.data);
        r_b->valid_in[porta_b](link.valid);
        r_b->ready_in[porta_b](link.ready);
    }

    // =========================================================
    // Sinais dummy para portas não utilizadas dos roteadores
    // raiz (LOCAL_0, LOCAL_1, UP_0, UP_1 de R4..R7)
    // Dimensão: [4 raízes][6 portas]
    // =========================================================
    sc_signal<Flit> sig_dummy_data[4][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_valid[4][NUM_TOTAL_PORTAS];
    sc_signal<Flit> sig_dummy_data_out[4][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_valid_out[4][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_ready[4][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_ready_in[4][NUM_TOTAL_PORTAS];

    // =========================================================
    // Destrutor
    // =========================================================
    ~SpinNocTop() {
        for (int i = 0; i < 8; i++) delete routers[i];
    }
};

#endif // SPIN_NOC_TOP_H
