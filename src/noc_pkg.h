#ifndef NOC_PKG_H
#define NOC_PKG_H

#include <systemc.h>

// Definição das portas do roteador SPIN (Fat-Tree)
enum PortID {
    LOCAL_0 = 0, // Conexão com Host A
    LOCAL_1 = 1, // Conexão com Host B
    UP_0    = 2, // Link para roteador pai superior 0
    UP_1    = 3, // Link para roteador pai superior 1
    DOWN_0  = 4, // Usado nos roteadores Raiz para descer (Subárvore Esquerda)
    DOWN_1  = 5  // Usado nos roteadores Raiz para descer (Subárvore Direita)
};

// Estrutura do Pacote de Dados (Flit único para simplificar a implementação)
struct Flit {
    int src_id;       // ID do host de origem (0 a 7)
    int dest_id;      // ID do host de destino (0 a 7)
    int payload;      // Dado útil transmitido
    sc_time timestamp;// Momento em que o flit foi gerado (para calcular latência)

    // Operador de igualdade obrigatório para sinais do SystemC
    bool operator==(const Flit& other) const {
        return (src_id == other.src_id) && 
               (dest_id == other.dest_id) && 
               (payload == other.payload) && 
               (timestamp == other.timestamp);
    }
};

// Função para permitir rastreio de ondas VCD do Struct customizado
inline void sc_trace(sc_trace_file* tf, const Flit& f, const std::string& name) {
    sc_trace(tf, f.src_id, name + ".src_id");
    sc_trace(tf, f.dest_id, name + ".dest_id");
    sc_trace(tf, f.payload, name + ".payload");
}

#endif