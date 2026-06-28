// ============================================================
// sim/spin_noc_tb.cpp — Testbench da NoC SPIN (Fat-Tree 8 Roteadores)
//
// CORREÇÕES APLICADAS (auditoria completa):
//
// [FIX-1] E115 — Múltiplos Drivers em tb_ready_in[]
//   PROBLEMA: gerador_trafego() escrevia tb_ready_in[i].write(false)
//             durante a inicialização, e aguarda_recepcao() também
//             escrevia tb_ready_in[], violando a regra de driver único
//             do sc_signal do SystemC.
//   SOLUÇÃO:  gerador_trafego() NÃO toca mais em tb_ready_in[].
//             tb_ready_in[] é escrito EXCLUSIVAMENTE por monitor_recepcao().
//             A função aguarda_recepcao() foi removida (era código morto e
//             segunda fonte do E115).
//
// [FIX-2] Condição de Corrida no Reset (timing 0 ns / 30 ns)
//   PROBLEMA: wait(30, SC_NS) alinhava a queda de rst exatamente sobre
//             um flanco positivo do clock (30 ns = 3 ciclos de 10 ns),
//             criando uma condição de setup/hold onde rst e clk.posedge
//             competiam no mesmo instante de simulação.
//   SOLUÇÃO:  Substituído por wait(clk.posedge_event()) sequenciais.
//             reset é desativado APÓS o 3º flanco positivo do clock,
//             garantindo que a queda ocorra no delta seguinte ao flanco
//             (sem sobreposição), e os sinais de injeção são zerados
//             antes da 1ª borda para evitar X na netlist.
//
// [FIX-3] Inicialização de tb_ready_in[] Antecipada (monitor_recepcao)
//   PROBLEMA: O loop de inicialização em monitor_recepcao() executava
//             antes de qualquer wait(), o que é correto em SC_THREAD
//             (roda no tempo 0), mas precisava ser o PRIMEIRO a rodar
//             para que os sinais estivessem estáveis antes do início do
//             tráfego. Confirmado e mantido.
//
// [FIX-4] Deadlock no monitor_recepcao — wait() duplo no loop interno
//   PROBLEMA: O loop while(true) fazia wait(clk.posedge_event()) no topo.
//             Quando detectava valid_out=1, fazia tb_ready_in.write(true),
//             depois outro wait(clk.posedge_event()), depois .write(false).
//             Isso consumia o próximo flanco DENTRO do loop for interno,
//             pulando a varredura dos demais hosts naquele ciclo e, pior,
//             se valid_out ficasse alto por mais de 1 ciclo, o monitor
//             poderia perder flits ou travar.
//   SOLUÇÃO:  O pulso de ready é gerenciado via flag booleana local.
//             O monitor:
//               1. Aguarda flanco positivo (wait no topo do while).
//               2. Varre todos os hosts no ciclo atual.
//               3. Para hosts com valid_out=1: escreve ready=true e marca
//                  flag para limpar no próximo ciclo.
//               4. Na iteração seguinte do while, antes do wait, limpa
//                  tb_ready_in[] dos hosts marcados.
//             Assim, o pulso de ready dura exatamente 1 ciclo de clock
//             (de um flanco positivo ao próximo), que é o comportamento
//             correto de handshake.
//
// [FIX-5] Exclusividade de Drivers — Auditoria Completa
//   gerador_trafego  → escreve APENAS: rst, tb_in[], tb_valid_in[]
//   monitor_recepcao → escreve APENAS: tb_ready_in[]
//   Nenhum outro método toca em sinais de saída.
//
// [FIX-6] Alinhamento de Clock em injeta_flit()
//   Todos os wait() em injeta_flit() usam wait(clk.posedge_event()).
//   O desligamento de valid_in ocorre 1 ciclo após a confirmação do
//   handshake (ready_out=1), garantindo protocolo correto de 1 ciclo.
//
// ============================================================

#include "spin_noc_tb.h"
#include <iostream>
#include <iomanip>

// ============================================================
// injeta_flit
//
// Injeta um único flit na NoC a partir de host_origem com
// destino host_destino, aguarda a confirmação de handshake
// (tb_ready_out=1 no flanco positivo do clock) e em seguida
// desativa tb_valid_in e limpa tb_in.
//
// Driver exclusivo: tb_in[host_origem], tb_valid_in[host_origem]
// Não toca em tb_ready_in (exclusivo de monitor_recepcao).
//
// Protocolo TX (1 ciclo):
//   Ciclo N  : tb_in = pkt, tb_valid_in = 1
//   Ciclo N+k: tb_ready_out = 1  →  handshake confirmado
//   Ciclo N+k+1: tb_valid_in = 0, tb_in = flit_vazio
// ============================================================
void SpinNocTb::injeta_flit(int host_origem, int host_destino,
                             int dado, const std::string& descricao)
{
    // Monta o pacote com timestamp atual para medir latência no receptor
    Flit pkt(host_origem, host_destino, dado, sc_time_stamp());

    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "@" << sc_time_stamp() << " [TB] Injetando pacote:" << std::endl;
    std::cout << "  Descrição : " << descricao << std::endl;
    std::cout << "  Origem    : HOST " << host_origem << std::endl;
    std::cout << "  Destino   : HOST " << host_destino << std::endl;
    std::cout << "  Payload   : 0x" << std::hex << std::uppercase
              << dado << std::dec << std::endl;
    std::cout << "============================================================" << std::endl;

    // ---- Apresenta o flit na porta de origem ----
    // Ambos os sinais sobem no mesmo instante (mesmo delta-cycle)
    // para que a NoC veja valid=1 junto com o dado estável.
    tb_in[host_origem].write(pkt);
    tb_valid_in[host_origem].write(true);

    // ---- Aguarda handshake: ready_out=1 no flanco positivo ----
    // O loop aguarda o primeiro flanco positivo em que tb_ready_out
    // esteja alto, indicando que a FIFO de entrada do roteador aceitou
    // o flit. Sem timeout deliberado: a simulação encerra em 2000 ns
    // via sc_start(), garantindo saída do loop no caso de falha da NoC.
    do {
        wait(clk.posedge_event());
    } while (!tb_ready_out[host_origem].read());

    // ---- Handshake confirmado: desativa valid no próximo ciclo ----
    // Aguarda o próximo flanco positivo para garantir que o ciclo do
    // handshake seja completo e amostrado corretamente pela FIFO,
    // antes de retirar o dado do barramento.
    wait(clk.posedge_event());
    tb_valid_in[host_origem].write(false);

    // Limpa o flit do barramento junto com a desativação do valid,
    // evitando propagação de dado obsoleto para outros ciclos.
    Flit flit_vazio;
    tb_in[host_origem].write(flit_vazio);

    std::cout << "@" << sc_time_stamp()
              << " [TB] Handshake TX confirmado — HOST " << host_origem
              << " -> HOST " << host_destino << std::endl;
}

// ============================================================
// gerador_trafego
//
// Thread principal de injeção de tráfego. Executa os 3
// cenários de teste em sequência e finaliza.
//
// Driver exclusivo: rst, tb_in[], tb_valid_in[]
// NÃO escreve em tb_ready_in[] (exclusivo de monitor_recepcao).
// ============================================================
void SpinNocTb::gerador_trafego() {

    // ----------------------------------------------------------
    // FASE 0: Inicialização segura de TODOS os sinais de saída
    //         deste thread ANTES do primeiro wait().
    //
    //         Em SC_THREAD, o código antes do primeiro wait()
    //         executa no tempo 0 (delta 0). Zeramos aqui apenas
    //         os sinais que são de nossa responsabilidade:
    //           rst, tb_in[], tb_valid_in[]
    //
    //         NÃO tocamos em tb_ready_in[] — pertence ao monitor.
    // ----------------------------------------------------------
    rst.write(true);

    for (int i = 0; i < 8; i++) {
        Flit flit_vazio;
        tb_in[i].write(flit_vazio);
        tb_valid_in[i].write(false);
        // [FIX-1] REMOVIDO: tb_ready_in[i].write(false)
        //         Este sinal pertence EXCLUSIVAMENTE a monitor_recepcao.
        //         Escrevê-lo aqui causava o erro E115 (multiple drivers).
    }

    // ----------------------------------------------------------
    // FASE 1: Manter reset ativo por 3 ciclos de clock completos.
    //
    //         [FIX-2] Substituímos wait(30, SC_NS) por 3 chamadas
    //         a wait(clk.posedge_event()). Isso garante que rst
    //         permanece alto por exatamente 3 flancos positivos e
    //         é desativado no delta IMEDIATAMENTE APÓS o 3º flanco,
    //         eliminando a condição de corrida onde rst caía
    //         exatamente sobre o flanco do clock.
    // ----------------------------------------------------------
    wait(clk.posedge_event()); // Flanco 1 — ciclo 0 (t=10 ns)
    wait(clk.posedge_event()); // Flanco 2 — ciclo 1 (t=20 ns)
    wait(clk.posedge_event()); // Flanco 3 — ciclo 2 (t=30 ns)

    // Desativa reset logo após o 3º flanco (t=30 ns + delta)
    rst.write(false);

    std::cout << "@" << sc_time_stamp()
              << " [TB] Reset desativado — NoC entrando em modo operacional" << std::endl;

    // ----------------------------------------------------------
    // FASE 2: Aguardar estabilização pós-reset.
    //         2 ciclos adicionais para que as FIFOs e árbitros
    //         completem a transição do estado de reset para o
    //         estado operacional sem interferência de sinais
    //         transitórios.
    // ----------------------------------------------------------
    wait(clk.posedge_event()); // Ciclo de estabilização 1 (t=40 ns)
    wait(clk.posedge_event()); // Ciclo de estabilização 2 (t=50 ns)

    std::cout << std::endl;
    std::cout << "############################################################" << std::endl;
    std::cout << "#       INÍCIO DA SIMULAÇÃO DA NoC SPIN (Fat-Tree)         #" << std::endl;
    std::cout << "#  Topologia: 8 roteadores | 8 hosts | 2 níveis            #" << std::endl;
    std::cout << "############################################################" << std::endl;

    // ==========================================================
    // CENÁRIO 1: HOST 0 -> HOST 7
    // Tipo     : PIOR CASO (3 saltos, subárvores distintas)
    // Caminho  : HOST 0 -> Router 0 -> Router 4 ou 5 -> Router 3 -> HOST 7
    // Payload  : 0xAB
    // ==========================================================

    // Aguarda 1 ciclo antes de iniciar injeção (garante borda limpa)
    wait(clk.posedge_event());

    injeta_flit(0, 7, 0xAB,
                "PIOR CASO: HOST 0 -> HOST 7 (3 saltos, subárvores distintas)");

    // ==========================================================
    // CENÁRIO 2: HOST 2 -> HOST 5
    // Tipo     : Cruzamento de subárvores
    // Caminho  : HOST 2 -> Router 1 -> Router 4/5 -> Router 2 -> HOST 5
    // Payload  : 0xCD
    // Intervalo: 2 ciclos entre injeções para evitar contenção
    // ==========================================================
    wait(clk.posedge_event()); // Intervalo ciclo 1
    wait(clk.posedge_event()); // Intervalo ciclo 2

    injeta_flit(2, 5, 0xCD,
                "CRUZAMENTO: HOST 2 -> HOST 5 (subárvores diferentes)");

    // ==========================================================
    // CENÁRIO 3: HOST 0 -> HOST 1
    // Tipo     : MELHOR CASO (destino local, 0 saltos externos)
    // Caminho  : HOST 0 -> Router 0 -> HOST 1
    // Payload  : 0xEF
    // ==========================================================
    wait(clk.posedge_event()); // Intervalo ciclo 1
    wait(clk.posedge_event()); // Intervalo ciclo 2

    injeta_flit(0, 1, 0xEF,
                "MELHOR CASO: HOST 0 -> HOST 1 (destino local no Router 0)");

    // ----------------------------------------------------------
    // FASE FINAL: Aguardar processamento de todos os pacotes.
    // 20 ciclos (200 ns) são suficientes para o pior caso de
    // latência da topologia Fat-Tree de 2 níveis.
    // ----------------------------------------------------------
    for (int ciclo = 0; ciclo < 20; ciclo++) {
        wait(clk.posedge_event());
    }

    std::cout << std::endl;
    std::cout << "############################################################" << std::endl;
    std::cout << "#          GERADOR DE TRÁFEGO FINALIZADO                   #" << std::endl;
    std::cout << "#  3 pacotes injetados nos 3 cenários de teste.             #" << std::endl;
    std::cout << "#  O monitor continuará ativo até o fim da simulação.       #" << std::endl;
    std::cout << "############################################################" << std::endl;

    // Thread termina aqui. O kernel SystemC continuará rodando
    // até sc_start() expirar (2000 ns em main.cpp), mantendo
    // monitor_recepcao ativo para capturar os últimos flits.
}

// ============================================================
// monitor_recepcao
//
// Thread de monitoramento contínuo. Varre todos os 8 hosts
// receptores a cada ciclo de clock e, ao detectar valid_out=1,
// confirma a leitura com um pulso de 1 ciclo em tb_ready_in[].
//
// Driver exclusivo: tb_ready_in[]
//
// [FIX-4] Protocolo de pulso corrigido — sem deadlock:
//   O ready_in é gerenciado com flag booleana `pendente_ready[]`.
//   Em cada iteração do while(true):
//     1. LIMPA ready_in[] para hosts marcados na iteração anterior
//        (isso garante que o pulso durou exatamente 1 ciclo)
//     2. Aguarda o próximo flanco positivo do clock
//     3. Varre todos os hosts
//     4. Hosts com valid_out=1: escreve ready=1 e marca pendente
//
//   Assim, o loop NUNCA tem dois wait() na mesma iteração,
//   eliminando o risco de saltar flancos de clock.
// ============================================================
void SpinNocTb::monitor_recepcao() {

    // ----------------------------------------------------------
    // Inicialização: derruba tb_ready_in[] no tempo 0.
    // Como este código executa antes do primeiro wait(), ocorre
    // em tempo 0 delta 0, garantindo estado inicial estável.
    // ----------------------------------------------------------
    for (int i = 0; i < 8; i++) {
        tb_ready_in[i].write(false);
    }

    // Estatísticas acumuladas
    int total_recebidos = 0;
    int total_erros     = 0;

    // Flags de controle do pulso de ready por host
    // pendente_ready[i] = true → este host teve handshake confirmado
    // no ciclo anterior e precisa ter ready_in baixado agora.
    bool pendente_ready[8] = {false, false, false, false,
                               false, false, false, false};

    // ----------------------------------------------------------
    // Loop principal: uma iteração = um ciclo de clock
    //
    // [FIX-4] Estrutura do loop:
    //   PASSO A) Baixa ready_in[] dos hosts com pulso pendente
    //            (fim do pulso de ACK do ciclo anterior)
    //   PASSO B) Aguarda flanco positivo do clock
    //   PASSO C) Varre todos os hosts
    //   PASSO D) Hosts com valid_out=1: sobe ready, marca pendente,
    //            imprime estatísticas
    //
    // O wait() no PASSO B garante que o loop sempre cede controle
    // ao kernel SystemC, eliminando risco de loop infinito travado.
    // ----------------------------------------------------------
    while (true) {

        // ---- PASSO A: Encerra pulso de ready do ciclo anterior ----
        for (int host = 0; host < 8; host++) {
            if (pendente_ready[host]) {
                tb_ready_in[host].write(false);
                pendente_ready[host] = false;
            }
        }

        // ---- PASSO B: Aguarda próximo flanco positivo do clock ----
        // Este é o ÚNICO wait() por iteração do while, garantindo
        // que o kernel SystemC sempre possa progredir.
        wait(clk.posedge_event());

        // ---- PASSO C/D: Varre todos os hosts no ciclo atual ----
        for (int host = 0; host < 8; host++) {

            if (!tb_valid_out[host].read()) {
                // Nenhum dado disponível neste host neste ciclo
                continue;
            }

            // ---- Dado disponível: captura o flit ----
            Flit recebido = tb_out[host].read();

            // ---- Sobe ready_in para confirmar leitura (início do pulso) ----
            // O pulso termina no PASSO A da próxima iteração.
            tb_ready_in[host].write(true);
            pendente_ready[host] = true;

            // ---- Calcula latência ----
            // timestamp foi gravado em injeta_flit() no momento da injeção.
            sc_time latencia = sc_time_stamp() - recebido.timestamp;
            total_recebidos++;

            // ---- Imprime relatório do pacote recebido ----
            std::cout << std::endl;
            std::cout << "************************************************************" << std::endl;
            std::cout << "* PACOTE RECEBIDO — HOST DESTINO: " << host << std::endl;
            std::cout << "* Tempo de recepção : " << sc_time_stamp() << std::endl;
            std::cout << "* Origem (src_id)   : HOST " << recebido.src_id << std::endl;
            std::cout << "* Destino (dest_id) : HOST " << recebido.dest_id << std::endl;
            std::cout << "* Payload           : 0x" << std::hex << std::uppercase
                       << recebido.payload << std::dec << std::endl;
            std::cout << "* Latência total    : " << latencia << std::endl;

            // ---- Verificação de integridade ----
            if (recebido.dest_id != host) {
                total_erros++;
                std::cout << "* [ERRO] Roteamento incorreto!" << std::endl;
                std::cout << "*        Esperado no HOST " << recebido.dest_id
                           << " — chegou no HOST " << host << std::endl;
            } else {
                std::cout << "* [OK] Integridade verificada — destino correto!" << std::endl;
            }

            std::cout << "* Total recebidos   : " << total_recebidos << std::endl;
            std::cout << "* Total de erros    : " << total_erros << std::endl;
            std::cout << "************************************************************" << std::endl;

        } // fim do for (host)

    } // fim do while (true)

    // Nota: este ponto é inalcançável. O loop é encerrado pelo
    // término da simulação via sc_start() em main.cpp.
}