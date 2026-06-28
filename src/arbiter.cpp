#include "arbiter.h"
#include <iostream>

// ============================================================
// arbitra
// Executa um ciclo de arbitragem Round-Robin a cada borda de
// subida do clock. Garante que nenhuma porta sofra starvation.
// ============================================================
void Arbiter::arbitra() {
    if (rst.read()) {
        // Reinicia ponteiro e desativa grant
        prioridade = 0;
        grant_out.write(-1);
        grant_valido.write(false);
        return;
    }

    int  porta_vencedora = -1;
    bool encontrou       = false;

    // Varre circularmente a partir do ponteiro de prioridade
    for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
        int candidato = (prioridade + i) % NUM_TOTAL_PORTAS;
        if (req_in[candidato].read()) {
            porta_vencedora = candidato;
            encontrou       = true;
            break;
        }
    }

    if (encontrou) {
        grant_out.write(porta_vencedora);
        grant_valido.write(true);
        // Avança o ponteiro para a próxima porta após a vencedora
        // Isso garante justiça Round-Robin no próximo ciclo
        prioridade = (porta_vencedora + 1) % NUM_TOTAL_PORTAS;
    } else {
        // Nenhuma requisição ativa neste ciclo
        grant_out.write(-1);
        grant_valido.write(false);
        // Mantém ponteiro inalterado (sem vencedor, sem avanço)
    }
}
