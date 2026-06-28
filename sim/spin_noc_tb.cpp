#include "spin_noc_tb.h"
#include <iostream>
#include <iomanip>

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
    std::cout << "  Payload   : 0x" << std::hex << std::uppercase
              << dado << std::dec << std::endl;
    std::cout << "============================================================" << std::endl;

    tb_in[host_origem].write(pkt);
    tb_valid_in[host_origem].write(true);

    do {
        wait(clk.posedge_event());
    } while (!tb_ready_out[host_origem].read());

    tb_valid_in[host_origem].write(false);
    Flit flit_vazio;
    tb_in[host_origem].write(flit_vazio);

    std::cout << "@" << sc_time_stamp()
              << " [TB] Handshake TX confirmado — HOST " << host_origem
              << " -> HOST " << host_destino << std::endl;
}

void SpinNocTb::gerador_trafego() {

    rst.write(true);

    for (int i = 0; i < 8; i++) {
        Flit flit_vazio;
        tb_in[i].write(flit_vazio);
        tb_valid_in[i].write(false);
    }

    wait(clk.posedge_event()); 
    wait(clk.posedge_event()); 
    wait(clk.posedge_event()); 

    rst.write(false);

    std::cout << "@" << sc_time_stamp()
              << " [TB] Reset desativado — NoC entrando em modo operacional" << std::endl;

    wait(clk.posedge_event()); 
    wait(clk.posedge_event()); 

    std::cout << std::endl;
    std::cout << "############################################################" << std::endl;
    std::cout << "#       INÍCIO DA SIMULAÇÃO DA NoC SPIN (Fat-Tree)         #" << std::endl;
    std::cout << "#  Topologia: 8 roteadores | 8 hosts | 2 níveis            #" << std::endl;
    std::cout << "############################################################" << std::endl;

    wait(clk.posedge_event());

    injeta_flit(0, 7, 0xAB,
                "PIOR CASO: HOST 0 -> HOST 7 (3 saltos, subárvores distintas)");

    wait(clk.posedge_event()); 
    wait(clk.posedge_event()); 

    injeta_flit(2, 5, 0xCD,
                "CRUZAMENTO: HOST 2 -> HOST 5 (subárvores diferentes)");

    wait(clk.posedge_event()); 
    wait(clk.posedge_event()); 

    injeta_flit(0, 1, 0xEF,
                "MELHOR CASO: HOST 0 -> HOST 1 (destino local no Router 0)");

    for (int ciclo = 0; ciclo < 20; ciclo++) {
        wait(clk.posedge_event());
    }

    std::cout << std::endl;
    std::cout << "############################################################" << std::endl;
    std::cout << "#          GERADOR DE TRÁFEGO FINALIZADO                   #" << std::endl;
    std::cout << "#  3 pacotes injetados nos 3 cenários de teste.             #" << std::endl;
    std::cout << "#  O monitor continuará ativo até o fim da simulação.       #" << std::endl;
    std::cout << "############################################################" << std::endl;
}

void SpinNocTb::monitor_recepcao() {

    for (int i = 0; i < 8; i++) {
        tb_ready_in[i].write(false);
    }

    int total_recebidos = 0;
    int total_erros     = 0;

    while (true) {
        wait(clk.posedge_event());

        for (int host = 0; host < 8; host++) {
            bool valid = tb_valid_out[host].read();
            bool ready = tb_ready_in[host].read();

            // O handshake só ocorre no exato ciclo em que ambos (valid e ready) estão em HIGH
            if (valid && ready) {
                Flit recebido = tb_out[host].read();
                
                sc_time latencia = sc_time_stamp() - recebido.timestamp;
                total_recebidos++;

                std::cout << std::endl;
                std::cout << "************************************************************" << std::endl;
                std::cout << "* PACOTE RECEBIDO — HOST DESTINO: " << host << std::endl;
                std::cout << "* Tempo de recepção : " << sc_time_stamp() << std::endl;
                std::cout << "* Origem (src_id)   : HOST " << recebido.src_id << std::endl;
                std::cout << "* Destino (dest_id) : HOST " << recebido.dest_id << std::endl;
                std::cout << "* Payload           : 0x" << std::hex << std::uppercase
                           << recebido.payload << std::dec << std::endl;
                std::cout << "* Latência total    : " << latencia << std::endl;

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

                // Baixa o ready após ler o flit para não duplicar leituras
                tb_ready_in[host].write(false);
            } 
            else if (valid && !ready) {
                // Dado disponível: sobe o ready para capturar no próximo ciclo de clock
                tb_ready_in[host].write(true);
            }
            else if (!valid && ready) {
                // Nenhum dado: garante que o ready fique em baixo
                tb_ready_in[host].write(false);
            }
        } 
    } 
}
