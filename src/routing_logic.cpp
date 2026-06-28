#include "routing_logic.h"

// TODO: Implementar algoritmo SPIN baseado nas seguintes regras matemáticas mecânicas:
// 1. Ler o dest_id do flit da entrada.
// 2. SE o ID do roteador atual for de 0 a 3 (Roteador Folha):
//    - Se (dest_id / 2) == router_id, significa que o destino está neste mesmo roteador!
//      -> Se dest_id for PAR, mande para a porta LOCAL_0.
//      -> Se dest_id for ÍMPAR, mande para a porta LOCAL_1.
//    - Se (dest_id / 2) != router_id, o destino está longe (fora da subárvore):
//      -> Alterne o envio entre as portas UP_0 e UP_1 usando uma variável booleana interna (Toggle) para balanceamento de carga Round-Robin.
//
// 3. SE o ID do roteador atual for de 4 a 7 (Roteador Raiz):
//    - Roteadores raiz só mandam para baixo (DOWN).
//    - Se dest_id for de 0 a 3 -> Envie para a porta DOWN_0 (Subárvore Esquerda).
//    - Se dest_id for de 4 a 7 -> Envie para a porta DOWN_1 (Subárvore Direita).
// 4. Ativar a flag de requisição para o Árbitro da porta selecionada.