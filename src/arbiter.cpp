#include "arbiter.h"

// TODO: Implementar Árbitro Round-Robin seguindo este passo a passo:
// 1. Crie uma variável interna global chamada 'prioridade' inicializada em 0.
// 2. A cada ciclo de clock, verifique o vetor de requisições de rota vindo das FIFOs.
// 3. Começando a varredura a partir do índice apontado por 'prioridade', o primeiro que tiver com requisição ativa ganha o canal (Grant).
// 4. Atualize a variável 'prioridade' para: (ganhador + 1) % NUM_TOTAL_PORTAS.
// 5. Isso garante que nenhuma porta sofra Starvation (fique esperando para sempre).