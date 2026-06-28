#include "crossbar.h"

// ============================================================
// chaveia
// Para cada porta de saída j, verifica o grant emitido pelo
// árbitro e conecta a entrada vencedora à saída correspondente.
// Propaga backpressure (ready) de volta à FIFO de entrada.
// ============================================================
void Crossbar::chaveia() {
    // Inicializa vetores de resultado
    Flit flit_vazio;
    for (int j = 0; j < NUM_TOTAL_PORTAS; j++) {
        flit_out[j].write(flit_vazio);
        valid_out[j].write(false);
    }
    for (int i = 0; i < NUM_TOTAL_PORTAS; i++) {
        ready_in[i].write(false);
    }

    // Para cada porta de saída, aplica o resultado da arbitragem
    for (int j = 0; j < NUM_TOTAL_PORTAS; j++) {
        if (grant_ok[j].read()) {
            int entrada = grant[j].read();

            // Validação: índice de entrada dentro do range esperado
            if (entrada >= 0 && entrada < NUM_TOTAL_PORTAS) {
                // Chaveamento de dado e válido
                flit_out[j].write(flit_in[entrada].read());
                valid_out[j].write(valid_in[entrada].read());

                // Propagação de backpressure: a saída j aceita?
                // -> Repassa para a FIFO de entrada vencedora
                ready_in[entrada].write(ready_out[j].read());
            }
        }
    }
}
