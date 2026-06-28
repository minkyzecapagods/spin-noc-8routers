#include "fifo_buffer.h"

// TODO: Implementar os métodos usando std::queue<Flit> interna:
// 1. Método Write: Se a fila interna.size() < CAPACIDADE (ex: 4 flits), insere o flit e atualiza flag 'full' para falso. Caso contrário, 'full' = true.
// 2. Método Read: Se a fila não estiver vazia, extrai o item do topo (.pop()), e se .size() == 0 atualiza a flag 'empty' para true.