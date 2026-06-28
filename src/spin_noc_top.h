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
//
//            Portas inativas (precisam de sinais dummy para
//            satisfazer o binding obrigatório do SystemC):
//              Roteadores FOLHA (0-3): DOWN_0 e DOWN_1
//                (não existe um terceiro nível de árvore abaixo)
//              Roteadores RAIZ  (4-7): LOCAL_0, LOCAL_1, UP_0, UP_1
//                (não há hosts locais nem nível acima da raiz)
// ============================================================

// ============================================================
// Estrutura auxiliar para agrupar os 3 sinais de UM SENTIDO de
// uma conexão ponto-a-ponto entre duas portas de roteadores
// ============================================================
struct LinkSinais {
    sc_signal<Flit> data;
    sc_signal<bool> valid;
    sc_signal<bool> ready;
};

// ============================================================
// Estrutura de um link FULL-DUPLEX entre duas portas físicas
// de roteadores distintos. Cada porta do roteador (ex.: UP_0)
// possui handshake de ENTRADA e de SAÍDA simultaneamente, logo
// uma conexão completa entre duas portas exige DOIS conjuntos
// de sinais independentes — um para cada sentido do tráfego:
//
//   ida   -> sinais usados no sentido r_a (saída) -> r_b (entrada)
//   volta -> sinais usados no sentido r_b (saída) -> r_a (entrada)
//
// Usar apenas um conjunto (como em uma versão simplex) deixa a
// metade das portas de cada lado sem binding, causando o erro
// E109 (complete binding failed) no SystemC.
// ============================================================
struct LinkBidirecional {
    LinkSinais ida;   // r_a -> r_b
    LinkSinais volta; // r_b -> r_a
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
    // Links internos da topologia Fat-Tree (full-duplex)
    // Nomenclatura: link_Ri_Pj_Rk_Pl
    //   Ri porta Pj <-> Rk porta Pl (bidirecional)
    // =========================================================

    // Links das folhas com as raízes (UP da folha <-> DOWN da raiz)
    // Router 0 <-> Router 4 e Router 5
    LinkBidirecional link_R0_UP0_R4_DOWN0; // Router 0 UP_0  <-> Router 4 DOWN_0
    LinkBidirecional link_R0_UP1_R5_DOWN0; // Router 0 UP_1  <-> Router 5 DOWN_0
    // Router 1 <-> Router 4 e Router 5
    LinkBidirecional link_R1_UP0_R4_DOWN1; // Router 1 UP_0  <-> Router 4 DOWN_1
    LinkBidirecional link_R1_UP1_R5_DOWN1; // Router 1 UP_1  <-> Router 5 DOWN_1
    // Router 2 <-> Router 6 e Router 7
    LinkBidirecional link_R2_UP0_R6_DOWN0; // Router 2 UP_0  <-> Router 6 DOWN_0
    LinkBidirecional link_R2_UP1_R7_DOWN0; // Router 2 UP_1  <-> Router 7 DOWN_0
    // Router 3 <-> Router 6 e Router 7
    LinkBidirecional link_R3_UP0_R6_DOWN1; // Router 3 UP_0  <-> Router 6 DOWN_1
    LinkBidirecional link_R3_UP1_R7_DOWN1; // Router 3 UP_1  <-> Router 7 DOWN_1

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
        //
        //    Cada chamada de conecta_link() liga as 12 portas
        //    envolvidas na conexão (6 de cada roteador: in e
        //    out, nos dois sentidos do tráfego), usando os 2
        //    conjuntos de sinais do LinkBidirecional (ida e
        //    volta). Isso garante que NENHUMA porta de UP/DOWN
        //    fique sem binding.
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
        // 4. Conecta as portas NÃO UTILIZADAS de TODOS os
        //    roteadores a sinais dummy, para evitar o erro
        //    de binding incompleto do SystemC (E109).
        //
        //    Roteadores FOLHA (0-3): não existe um terceiro
        //    nível de árvore abaixo deles, então as portas
        //    DOWN_0 e DOWN_1 ficam inativas.
        //
        //    Roteadores RAIZ (4-7): não possuem hosts locais
        //    nem link UP para um nível acima da raiz, então as
        //    portas LOCAL_0, LOCAL_1, UP_0 e UP_1 ficam
        //    inativas.
        //
        //    É obrigatório conectar todos os sc_in/sc_out
        //    restantes para evitar erros de binding do SystemC.
        // --------------------------------------------------
        for (int r = 0; r < 8; r++) {
            bool eh_folha = (r < 4);

            // Lista de portas inativas para este roteador
            int portas_nao_usadas[4];
            int qtd_portas;

            if (eh_folha) {
                // Roteador folha: somente DOWN_0 e DOWN_1 são inativas
                portas_nao_usadas[0] = (int)DOWN_0;
                portas_nao_usadas[1] = (int)DOWN_1;
                qtd_portas = 2;
            } else {
                // Roteador raiz: LOCAL_0, LOCAL_1, UP_0 e UP_1 são inativas
                portas_nao_usadas[0] = (int)LOCAL_0;
                portas_nao_usadas[1] = (int)LOCAL_1;
                portas_nao_usadas[2] = (int)UP_0;
                portas_nao_usadas[3] = (int)UP_1;
                qtd_portas = 4;
            }

            for (int k = 0; k < qtd_portas; k++) {
                int p = portas_nao_usadas[k];

                // Entradas do roteador vindas de "nenhum lugar"
                routers[r]->flit_in[p](sig_dummy_data[r][p]);
                routers[r]->valid_in[p](sig_dummy_valid[r][p]);
                // ready_in: roteador indica que pode receber (sink dummy)
                routers[r]->ready_in[p](sig_dummy_ready_in[r][p]);

                // Saídas do roteador indo para "nenhum lugar"
                routers[r]->flit_out[p](sig_dummy_data_out[r][p]);
                routers[r]->valid_out[p](sig_dummy_valid_out[r][p]);
                // ready_out: roteador recebe backpressure do "nenhum lugar" (sempre false)
                routers[r]->ready_out[p](sig_dummy_ready[r][p]);
            }
        }
    }

    // =========================================================
    // Método auxiliar: conecta as portas de DOIS roteadores
    // formando um link FULL-DUPLEX completo.
    //
    // r_a, porta_a -> roteador/porta de um lado do link
    // r_b, porta_b -> roteador/porta do outro lado do link
    //
    // Sentido "ida"   (r_a -> r_b): saída de r_a entra em r_b
    // Sentido "volta" (r_b -> r_a): saída de r_b entra em r_a
    //
    // As 12 portas envolvidas (6 de cada roteador) são todas
    // conectadas explicitamente — nenhuma fica para trás.
    // =========================================================
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

    // =========================================================
    // Sinais dummy para portas não utilizadas de QUALQUER um
    // dos 8 roteadores (folhas usam os índices DOWN_0/DOWN_1;
    // raízes usam os índices LOCAL_0/LOCAL_1/UP_0/UP_1).
    // Dimensão: [8 roteadores][NUM_TOTAL_PORTAS]
    // (somente as posições efetivamente inativas de cada
    //  roteador são conectadas; as demais simplesmente não
    //  são usadas, o que não gera nenhum problema, pois
    //  sc_signal não exige binding — apenas sc_port exige)
    // =========================================================
    sc_signal<Flit> sig_dummy_data[8][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_valid[8][NUM_TOTAL_PORTAS];
    sc_signal<Flit> sig_dummy_data_out[8][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_valid_out[8][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_ready[8][NUM_TOTAL_PORTAS];
    sc_signal<bool> sig_dummy_ready_in[8][NUM_TOTAL_PORTAS];

    // =========================================================
    // Destrutor
    // =========================================================
    ~SpinNocTop() {
        for (int i = 0; i < 8; i++) delete routers[i];
    }
};

#endif // SPIN_NOC_TOP_H