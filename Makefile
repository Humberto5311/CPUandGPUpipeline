# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
#
# Uso:  make          -> compila a biblioteca e roda o teste da fila
#       make lib      -> so a biblioteca (.so / .dll / .dylib)
#       make test     -> so o teste
#       make clean    -> apaga o que foi gerado

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CLINC    ?=

ifeq ($(OS),Windows_NT)
  LIB      := motor_grafico.dll
  TEST     := test_fila.exe
  LIBFLAGS := -shared -static-libgcc -static-libstdc++
  OPENCL   := -lOpenCL
else ifeq ($(shell uname -s),Darwin)
  LIB      := libmotor_grafico.dylib
  TEST     := test_fila
  LIBFLAGS := -dynamiclib
  OPENCL   := -framework OpenCL
else
  LIB      := libmotor_grafico.so
  TEST     := test_fila
  LIBFLAGS := -shared -fPIC
  OPENCL   := -lOpenCL
endif

all: lib test

lib: $(LIB)

$(LIB): processamento.cpp processamento.h
	$(CXX) $(CXXFLAGS) $(CLINC) $(LIBFLAGS) processamento.cpp -o $@ $(OPENCL)

test: $(TEST)
	./$(TEST)

$(TEST): test_fila.cpp processamento.h
	$(CXX) $(CXXFLAGS) -pthread test_fila.cpp -o $@

clean:
	rm -f $(LIB) $(TEST)

.PHONY: all lib test clean
