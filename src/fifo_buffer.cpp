#include "fifo_buffer.h"

// ============================================================
// processo_escrita
// Condição de escrita: din_valid=1 e fila não cheia (din_ready=1)
// Ocorre no flanco de subida do clock
// ============================================================
void FifoBuffer::processo_escrita() {
    if (rst.read()) {
        // Esvazia a fila em caso de reset
        while (!fila.empty()) fila.pop();
        return;
    }

    // Handshake satisfeito: produtor tem dado válido e FIFO tem espaço
    if (din_valid.read() && (int)fila.size() < CAPACIDADE_FIFO) {
        Flit novo_flit = din.read();
        fila.push(novo_flit);
        // Log de depuração (removível em síntese)
        // std::cout << "@" << sc_time_stamp()
        //           << " [FIFO] Flit inserido: " << novo_flit << std::endl;
    }
}

// ============================================================
// processo_leitura
// Remove o flit da frente da fila quando o consumidor
// sinaliza dout_ready=1 e há dado disponível (dout_valid=1)
// ============================================================
void FifoBuffer::processo_leitura() {
    if (rst.read()) return;

    if (!fila.empty() && dout_ready.read()) {
        // std::cout << "@" << sc_time_stamp()
        //           << " [FIFO] Flit removido: " << fila.front() << std::endl;
        fila.pop();
    }
}

// ============================================================
// atualiza_saidas
// Mantém saídas combinacionais atualizadas conforme estado interno
// din_ready  = 1 quando há espaço na fila (fila.size < CAPACIDADE)
// dout_valid = 1 quando há pelo menos 1 flit na fila
// dout       = flit da cabeça da fila (ou flit inválido se vazia)
// ============================================================
void FifoBuffer::atualiza_saidas() {
    if (rst.read()) {
        din_ready.write(false);
        dout_valid.write(false);
        Flit flit_vazio;
        dout.write(flit_vazio);
        return;
    }

    // Pronto para receber se não estiver cheia
    din_ready.write((int)fila.size() < CAPACIDADE_FIFO);

    // Válido para enviar se não estiver vazia
    dout_valid.write(!fila.empty());

    // Disponibiliza o flit da cabeça na porta de saída
    if (!fila.empty()) {
        dout.write(fila.front());
    } else {
        Flit flit_vazio;
        dout.write(flit_vazio);
    }
}
