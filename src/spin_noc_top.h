#ifndef SPIN_NOC_TOP_H
#define SPIN_NOC_TOP_H

#include <systemc.h>
#include "router.h"
#include "noc_pkg.h"

SC_MODULE(SpinNocTop) {
    sc_in<bool> clk;
    sc_in<bool> rst;

    // Portas expostas para o Testbench injetar tráfego diretamente nos 8 hosts
    sc_in<Flit>  tb_in[8];
    sc_in<bool>  tb_valid_in[8];
    sc_out<bool> tb_ready_out[8];
    
    sc_out<Flit> tb_out[8];
    sc_out<bool> tb_valid_out[8];
    sc_in<bool>  tb_ready_in[8];

    // Array de 8 Instâncias de Roteadores (0 a 3 = Leaf, 4 a 7 = Root)
    Router* routers[8];

    // Sinais internos para interconexão (Malha Fat-Tree)
    // Siga estritamente o mapa de fiação abaixo no construtor
    sc_signal<Flit>  sig_data[24];
    sc_signal<bool>  sig_valid[24];
    sc_signal<bool>  sig_ready[24];

    SC_CTOR(SpinNocTop) {
        for(int i=0; i<8; i++) {
            char name[20];
            sprintf(name, "Router_%d", i);
            routers[i] = new Router(name);
            routers[i]->clk(clk);
            routers[i]->rst(rst);
        }

        // ====================================================================
        // INSTRUÇÃO DE INSTALAÇÃO DA FIAÇÃO (Mapear os sig_data nas portas)
        // ====================================================================
        // 1. Conectar tb_in[0..7] nas portas LOCAL dos roteadores 0..3:
        //    - Router 0 (LOCAL_0 -> Host 0, LOCAL_1 -> Host 1)
        //    - Router 1 (LOCAL_0 -> Host 2, LOCAL_1 -> Host 3)
        //    - Router 2 (LOCAL_0 -> Host 4, LOCAL_1 -> Host 5)
        //    - Router 3 (LOCAL_0 -> Host 6, LOCAL_1 -> Host 7)
        //
        // 2. Conectar as portas de subida (UP_0, UP_1) das Folhas nas Raízes (DOWN_0, DOWN_1):
        //    - Router 0 UP_0 -> Router 4 DOWN_0
        //    - Router 0 UP_1 -> Router 5 DOWN_0
        //    - Router 1 UP_0 -> Router 4 DOWN_1
        //    - Router 1 UP_1 -> Router 5 DOWN_1
        //    - Router 2 UP_0 -> Router 6 DOWN_0
        //    - Router 2 UP_1 -> Router 7 DOWN_0
        //    - Router 3 UP_0 -> Router 6 DOWN_1
        //    - Router 3 UP_1 -> Router 7 DOWN_1
        // ====================================================================
    }

    ~SpinNocTop() {
        for(int i=0; i<8; i++) delete routers[i];
    }
};
#endif