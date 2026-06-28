# ====================================================================
# MAKEFILE PARA COMPILAÇÃO DO SYSTEMC
# ====================================================================
SYSTEMC_HOME ?= /usr/local/systemc-2.3.3

CXX      = g++
CXXFLAGS = -Wall -O2 -I. -I./src -I./sim -I$(SYSTEMC_HOME)/include -Wno-deprecated
LDFLAGS  = -L$(SYSTEMC_HOME)/lib-linux64 -lsystemc -lm

# Coleta de arquivos de origem
SRC_FILES = src/arbiter.cpp src/crossbar.cpp src/fifo_buffer.cpp \
            src/routing_logic.cpp sim/spin_noc_tb.cpp sim/main.cpp

OBJ_FILES = $(SRC_FILES:.cpp=.o)
TARGET    = sim_noc

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o sim/*.o $(TARGET) simulation_waveform.vcd