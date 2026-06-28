# ====================================================================
# MAKEFILE — NoC SPIN em SystemC (Fat-Tree com 8 Roteadores)
# Disciplina: Organização de Computadores
# ====================================================================
#
# USO:
#   make           — Compila o projeto completo
#   make run       — Compila e executa a simulação
#   make wave      — Abre o VCD no GTKWave (requer gtkwave instalado)
#   make clean     — Remove arquivos objeto e executável
#   make help      — Exibe este menu
#
# CONFIGURAÇÃO:
#   Defina $SYSTEMC_HOME apontando para o diretório raiz da sua
#   instalação do SystemC antes de rodar o make. Exemplo:
#
#     export SYSTEMC_HOME=/usr/local/systemc-2.3.3
#     make
#
#   Ou passe diretamente na linha de comando:
#
#     make SYSTEMC_HOME=/caminho/para/systemc
# ====================================================================

SYSTEMC_HOME ?= /usr/local/systemc-2.3.3

# --- Compilador e Flags ---
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 \
           -I. -I./src -I./sim \
           -I$(SYSTEMC_HOME)/include \
           -Wno-deprecated \
           -Wno-unused-parameter

# --- Linker Flags ---
# Detecta automaticamente se a lib é lib-linux64 ou lib-linux
SYSTEMC_LIB_DIR := $(shell \
  if [ -d "$(SYSTEMC_HOME)/lib-linux64" ]; then \
    echo "$(SYSTEMC_HOME)/lib-linux64"; \
  elif [ -d "$(SYSTEMC_HOME)/lib-linux" ]; then \
    echo "$(SYSTEMC_HOME)/lib-linux"; \
  else \
    echo "$(SYSTEMC_HOME)/lib"; \
  fi)

LDFLAGS  = -L$(SYSTEMC_LIB_DIR) -lsystemc -lm -Wl,-rpath,$(SYSTEMC_LIB_DIR)

# --- Arquivos Fonte ---
SRC_FILES = src/arbiter.cpp      \
            src/crossbar.cpp     \
            src/fifo_buffer.cpp  \
            src/routing_logic.cpp \
            sim/spin_noc_tb.cpp  \
            sim/main.cpp

OBJ_FILES = $(SRC_FILES:.cpp=.o)
TARGET    = sim_noc

# ====================================================================
# Regras
# ====================================================================

.PHONY: all run wave clean help

all: $(TARGET)
	@echo ""
	@echo ">>> Compilação concluída com sucesso! Execute: ./$(TARGET)"
	@echo ""

$(TARGET): $(OBJ_FILES)
	@echo "[LINK] Ligando módulos em $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	@echo "[CC]   Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Dependências de cabeçalho (evita recompilação desnecessária)
src/arbiter.o:       src/arbiter.cpp       src/arbiter.h       src/noc_pkg.h
src/crossbar.o:      src/crossbar.cpp      src/crossbar.h      src/noc_pkg.h
src/fifo_buffer.o:   src/fifo_buffer.cpp   src/fifo_buffer.h   src/noc_pkg.h
src/routing_logic.o: src/routing_logic.cpp src/routing_logic.h src/noc_pkg.h
sim/spin_noc_tb.o:   sim/spin_noc_tb.cpp   sim/spin_noc_tb.h   src/noc_pkg.h \
                     src/spin_noc_top.h    src/router.h
sim/main.o:          sim/main.cpp          src/spin_noc_top.h  sim/spin_noc_tb.h

run: all
	@echo ""
	@echo ">>> Iniciando simulação..."
	@echo ""
	./$(TARGET)

wave: simulation_waveform.vcd
	@echo ">>> Abrindo formas de onda no GTKWave..."
	gtkwave simulation_waveform.vcd &

simulation_waveform.vcd: run

clean:
	@echo ">>> Removendo arquivos objeto e executável..."
	rm -f src/*.o sim/*.o $(TARGET) simulation_waveform.vcd
	@echo ">>> Limpeza concluída."

help:
	@echo ""
	@echo "Uso do Makefile — NoC SPIN em SystemC"
	@echo "======================================="
	@echo "  make              Compila o projeto"
	@echo "  make run          Compila e executa"
	@echo "  make wave         Abre VCD no GTKWave"
	@echo "  make clean        Remove artefatos de build"
	@echo ""
	@echo "Variável de ambiente obrigatória:"
	@echo "  SYSTEMC_HOME=/caminho/para/systemc  (padrão: /usr/local/systemc-2.3.3)"
	@echo ""
