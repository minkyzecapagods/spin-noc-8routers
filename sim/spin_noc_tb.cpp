#include "spin_noc_tb.h"

void SpinNocTb::traffic_generator() {
    // Inicialização do sistema
    rst.write(true);
    wait(20, SC_NS);
    rst.write(false);
    wait(10, SC_NS);

    // ====================================================================
    // INSTRUÇÃO PASSO A PASSO PARA VALIDAÇÃO DO PIOR CASO
    // ====================================================================
    // Cenário: Host 0 enviando para o Host 7 (Caminho mais longo da árvore Fat-Tree)
    // O pacote deve subir: Router 0 -> Router 4 -> Router 3 -> Destino Host 7.
    
    cout << "@" << sc_time_stamp() << " [TB] Injetando pacote: Host 0 -> Host 7" << endl;
    
    Flit p1;
    p1.src_id = 0;
    p1.dest_id = 7;
    p1.payload = 100; // ID fictício do dado
    p1.timestamp = sc_time_stamp();

    // Injeta na porta local 0 (Roteador 0)
    tb_in[0].write(p1);
    tb_valid_in[0].write(true);
    
    // Aguarda até o roteador aceitar (Handshake)
    do { wait(clk.value_changed_event()); } while (!tb_ready_out[0].read());
    tb_valid_in[0].write(false);

    // Monitoramento da chegada no receptor (Host 7)
    while(true) {
        wait(clk.value_changed_event());
        if (tb_valid_out[7].read() == true) {
            Flit recebido = tb_out[7].read();
            sc_time latencia = sc_time_stamp() - recebido.timestamp;
            cout << "====================================================" << endl;
            cout << "SUCESSO! Pacote recebido no destino!" << endl;
            cout << "Dado: " << recebido.payload << " | Origem: " << recebido.src_id << endl;
            cout << "Latência medida na simulação: " << latencia << endl;
            cout << "====================================================" << endl;
            break;
        }
    }
}