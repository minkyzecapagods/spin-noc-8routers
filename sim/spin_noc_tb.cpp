#include "spin_noc_tb.h"
#include <iostream>
#include <iomanip>

// ============================================================
// Auxiliar: injeta um flit e aguarda confirmação de handshake
// ============================================================
void SpinNocTb::injeta_flit(int host_origem, int host_destino,
                             int dado, const std::string& descricao)
{
    Flit pkt(host_origem, host_destino, dado, sc_time_stamp());

    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "@" << sc_time_stamp() << " [TB] Injetando pacote:" << std::endl;
    std::cout << "  Descrição : " << descricao << std::endl;
    std::cout << "  Origem    : HOST " << host_origem << std::endl;
    std::cout << "  Destino   : HOST " << host_destino << std::endl;
    std::cout << "  Payload   : " << dado << std::endl;
    std::cout << "============================================================" << std::endl;

    // Apresenta o flit na porta de origem
    tb_in[host_origem].write(pkt);
    tb_valid_in[host_origem].write(true);

    // Aguarda até a NoC confirmar recepção (handshake ready)
    do {
        wait(clk.posedge_event());
    } while (!tb_ready_out[host_origem].read());

    // Desativa valid após handshake (protocolo de 1 ciclo)
    wait(clk.posedge_event());
    tb_valid_in[host_origem].write(false);
    Flit flit_vazio;
    tb_in[host_origem].write(flit_vazio);
}

// ============================================================
// Auxiliar: aguarda chegada de flit em host_destino
// Retorna true se recebeu dentro do timeout
// ============================================================
bool SpinNocTb::aguarda_recepcao(int host_destino, int timeout_ciclos,
                                  Flit& flit_recebido)
{
    for (int ciclo = 0; ciclo < timeout_ciclos; ciclo++) {
        wait(clk.posedge_event());
        if (tb_valid_out[host_destino].read()) {
            flit_recebido = tb_out[host_destino].read();
            // Confirma leitura (ready)
            tb_ready_in[host_destino].write(true);
            wait(clk.posedge_event());
            tb_ready_in[host_destino].write(false);
            return true;
        }
    }
    return false;
}

// ============================================================
// Thread: gerador_trafego
// Executa os cenários de teste em sequência
// ============================================================
void SpinNocTb::gerador_trafego() {
    // ----------------------------------------------------------
    // Inicialização de todas as portas de saída do TB
    // ----------------------------------------------------------
    rst.write(true);
    for (int i = 0; i < 8; i++) {
        Flit flit_vazio;
        tb_in[i].write(flit_vazio);
        tb_valid_in[i].write(false);
        tb_ready_in[i].write(false);
    }

    // ----------------------------------------------------------
    // Mantém reset por 3 ciclos (30 ns com clock de 10 ns)
    // ----------------------------------------------------------
    wait(30, SC_NS);
    rst.write(false);
    wait(20, SC_NS); // Aguarda estabilização pós-reset

    std::cout << std::endl;
    std::cout << "############################################################" << std::endl;
    std::cout << "#       INÍCIO DA SIMULAÇÃO DA NoC SPIN (Fat-Tree)         #" << std::endl;
    std::cout << "#  Topologia: 8 roteadores | 8 hosts | 2 níveis            #" << std::endl;
    std::cout << "############################################################" << std::endl;

    // ==========================================================
    // CENÁRIO 1: HOST 0 -> HOST 7 (PIOR CASO — 3 saltos)
    // Caminho esperado:
    //   HOST 0 -> Router 0 -> Router 4 (ou 5) -> Router 3 -> HOST 7
    // ==========================================================
    wait(10, SC_NS);
    injeta_flit(0, 7, 0xAB, "PIOR CASO: HOST 0 -> HOST 7 (3 saltos, subárvores distintas)");

    // ==========================================================
    // CENÁRIO 2: HOST 2 -> HOST 5 (cruzamento de subárvores)
    // Caminho esperado:
    //   HOST 2 -> Router 1 -> Router 4/5 -> Router 2 -> HOST 5
    // ==========================================================
    wait(20, SC_NS);
    injeta_flit(2, 5, 0xCD, "CRUZAMENTO: HOST 2 -> HOST 5 (subárvores diferentes)");

    // ==========================================================
    // CENÁRIO 3: HOST 0 -> HOST 1 (MELHOR CASO — destino local)
    // Caminho esperado:
    //   HOST 0 -> Router 0 -> HOST 1 (0 saltos externos)
    // ==========================================================
    wait(20, SC_NS);
    injeta_flit(0, 1, 0xEF, "MELHOR CASO: HOST 0 -> HOST 1 (destino local no Router 0)");

    // Aguarda todos os pacotes serem processados
    wait(200, SC_NS);

    std::cout << std::endl;
    std::cout << "############################################################" << std::endl;
    std::cout << "#          GERADOR DE TRÁFEGO FINALIZADO                   #" << std::endl;
    std::cout << "############################################################" << std::endl;
}

// ============================================================
// Thread: monitor_recepcao
// Monitora continuamente todos os 8 hosts receptores e
// imprime os flits recebidos com latência e verificação
// de integridade.
// ============================================================
void SpinNocTb::monitor_recepcao() {
    // Inicializa ready de todos os hosts como falso
    for (int i = 0; i < 8; i++) {
        tb_ready_in[i].write(false);
    }

    // Contadores de estatísticas
    int total_recebidos = 0;
    int total_erros     = 0;

    while (true) {
        wait(clk.posedge_event());

        for (int host = 0; host < 8; host++) {
            if (tb_valid_out[host].read()) {
                Flit recebido = tb_out[host].read();

                // Confirma leitura imediatamente
                tb_ready_in[host].write(true);
                wait(clk.posedge_event());
                tb_ready_in[host].write(false);

                // Calcula latência
                sc_time latencia = sc_time_stamp() - recebido.timestamp;

                total_recebidos++;

                std::cout << std::endl;
                std::cout << "************************************************************" << std::endl;
                std::cout << "* PACOTE RECEBIDO NO HOST " << host << std::endl;
                std::cout << "* Tempo atual    : " << sc_time_stamp() << std::endl;
                std::cout << "* Origem         : HOST " << recebido.src_id << std::endl;
                std::cout << "* Destino        : HOST " << recebido.dest_id << std::endl;
                std::cout << "* Payload        : 0x" << std::hex << std::uppercase
                           << recebido.payload << std::dec << std::endl;
                std::cout << "* Latência total : " << latencia << std::endl;

                // Verificação de integridade: destino bate com host receptor?
                if (recebido.dest_id != host) {
                    total_erros++;
                    std::cout << "* [ERRO] Flit recebido no host errado!" << std::endl;
                    std::cout << "*        Esperado: HOST " << recebido.dest_id
                               << " | Recebido em: HOST " << host << std::endl;
                } else {
                    std::cout << "* [OK] Integridade verificada - destino correto!" << std::endl;
                }

                std::cout << "************************************************************" << std::endl;
            }
        }
    }
}
