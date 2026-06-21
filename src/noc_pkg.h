#ifndef NOC_PKG_H
#define NOC_PKG_H

#include <systemc.h>

// TODO: DEFINIÇÕES GLOBAIS DA REDE
// 1. Definir a estrutura do Flit/Pacote usando um struct customizado (ex: id_origem, id_destino, payload, tipo_flit).
// 2. Usar 'sc_trace' para permitir que esse struct seja visualizado em arquivos de onda (.vcd).
// 3. Definir enums ou constantes para as portas (LOCAL = 0, UP0 = 1, UP1 = 2, DOWN0 = 3, DOWN1 = 4).

struct Flit {
    int src_id;
    int dest_id;
    int payload;
    // TODO: Adicionar campos necessários
};

#endif
