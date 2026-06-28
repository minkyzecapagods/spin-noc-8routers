#include "fifo_buffer.h"

// ============================================================
// processo_fifo
// Executa leitura, escrita e atualização de saídas de forma
// determinística, simulando corretamente um bloco de hardware.
// ============================================================
void FifoBuffer::processo_fifo() {
    if (rst.read()) {
        // Esvazia a fila em caso de reset
        while (!fila.empty()) fila.pop();
        
        din_ready.write(false);
        dout_valid.write(false);
        Flit flit_vazio;
        dout.write(flit_vazio);
        return;
    }

    bool pushed = false;
    bool popped = false;
    Flit incoming;

    // 1. Avalia as condições de entrada ANTES de modificar o estado interno
    if (din_valid.read() && fila.size() < CAPACIDADE_FIFO) {
        incoming = din.read();
        pushed = true;
    }
    
    if (!fila.empty() && dout_ready.read()) {
        popped = true;
    }

    // 2. Atualiza o estado da memória (a fila real)
    if (popped) {
        fila.pop();
    }
    
    if (pushed) {
        fila.push(incoming);
    }

    // 3. Atualiza as saídas simultaneamente para o próximo ciclo
    din_ready.write(fila.size() < CAPACIDADE_FIFO);
    dout_valid.write(!fila.empty());
    
    if (!fila.empty()) {
        dout.write(fila.front());
    } else {
        Flit flit_vazio;
        dout.write(flit_vazio);
    }
}
