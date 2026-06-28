#ifndef NOC_PKG_H
#define NOC_PKG_H

#include <systemc.h>
#include <iostream>

// ============================================================
// Número total de portas por roteador
// ============================================================
#define NUM_TOTAL_PORTAS 6
#define CAPACIDADE_FIFO  4

// ============================================================
// IDs de porta do roteador SPIN (Fat-Tree de 2 níveis)
// LOCAL_0 e LOCAL_1: Hosts locais conectados ao roteador folha
// UP_0 e UP_1: Links de subida para roteadores raiz
// DOWN_0 e DOWN_1: Links de descida (usados pelos roteadores raiz)
// ============================================================
enum PortID {
    LOCAL_0 = 0, // Conexão com Host A (par)
    LOCAL_1 = 1, // Conexão com Host B (ímpar)
    UP_0    = 2, // Link para roteador pai superior 0
    UP_1    = 3, // Link para roteador pai superior 1
    DOWN_0  = 4, // Link de descida para subárvore esquerda
    DOWN_1  = 5  // Link de descida para subárvore direita
};

// ============================================================
// Estrutura do pacote de dados (Flit único)
// Simplificação: cada flit carrega cabeçalho + dado útil
// ============================================================
struct Flit {
    int     src_id;    // ID do host de origem  (0 a 7)
    int     dest_id;   // ID do host de destino (0 a 7)
    int     payload;   // Dado útil transmitido
    sc_time timestamp; // Instante de criação (para cálculo de latência)
    bool    valido;    // Indica se este flit carrega dado real

    // Construtor padrão — flit inválido
    Flit() : src_id(-1), dest_id(-1), payload(0),
             timestamp(SC_ZERO_TIME), valido(false) {}

    // Construtor completo
    Flit(int src, int dest, int data, sc_time ts)
        : src_id(src), dest_id(dest), payload(data),
          timestamp(ts), valido(true) {}

    // Operador de igualdade obrigatório para sc_signal<Flit>
    bool operator==(const Flit& other) const {
        return (src_id    == other.src_id)  &&
               (dest_id   == other.dest_id) &&
               (payload   == other.payload) &&
               (timestamp == other.timestamp) &&
               (valido    == other.valido);
    }

    bool operator!=(const Flit& other) const {
        return !(*this == other);
    }
};

// ============================================================
// Operador de stream: permite imprimir o Flit no cout
// ============================================================
inline std::ostream& operator<<(std::ostream& os, const Flit& f) {
    os << "[Flit src=" << f.src_id
       << " dest="    << f.dest_id
       << " payload=" << f.payload
       << " ts="      << f.timestamp
       << " valido="  << (f.valido ? "sim" : "nao") << "]";
    return os;
}

// ============================================================
// Função de rastreio VCD para o struct customizado Flit
// ============================================================
inline void sc_trace(sc_trace_file* tf, const Flit& f, const std::string& name) {
    sc_trace(tf, f.src_id,  name + ".src_id");
    sc_trace(tf, f.dest_id, name + ".dest_id");
    sc_trace(tf, f.payload, name + ".payload");
    sc_trace(tf, f.valido,  name + ".valido");
}

#endif // NOC_PKG_H
