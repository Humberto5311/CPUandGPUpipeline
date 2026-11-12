# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

VERSION := $(shell cat VERSION 2>/dev/null || echo 0.0.0-dev)
CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic -fPIC
CXXFLAGS += -DAPP_VERSION=\"$(VERSION)\" -DPIPELINE_BUILD

UNAME_S := $(shell uname -s 2>/dev/null)
OPENCL_AVAILABLE := $(shell pkg-config --exists OpenCL 2>/dev/null && echo yes)
ifeq ($(OPENCL_AVAILABLE),yes)
  CXXFLAGS += -DPIPELINE_WITH_OPENCL $(shell pkg-config --cflags OpenCL)
  OPENCL_LIBS := $(shell pkg-config --libs OpenCL)
else
  OPENCL_LIBS :=
endif

ifeq ($(UNAME_S),Darwin)
  TARGET := libmotor_grafico.dylib
  SHARED := -dynamiclib
  ifeq ($(OPENCL_AVAILABLE),yes)
    OPENCL_LIBS := -framework OpenCL
  endif
else ifeq ($(OS),Windows_NT)
  TARGET := motor_grafico.dll
  SHARED := -shared
else
  TARGET := libmotor_grafico.so
  SHARED := -shared
endif

.PHONY: all clean test check-license info
all: $(TARGET)

$(TARGET): processamento.cpp processamento.h VERSION
	$(CXX) $(CXXFLAGS) $(SHARED) -o $@ processamento.cpp $(OPENCL_LIBS) -pthread

info:
	@echo "Versao: $(VERSION)"
	@echo "OpenCL: $(if $(filter yes,$(OPENCL_AVAILABLE)),habilitado,nao encontrado - fallback CPU)"

check-license:
	@missing=$$(find . \( -name '*.cpp' -o -name '*.h' -o -name '*.py' \) -not -path './.git/*' -exec grep -L "Mozilla Public" {} +); \
	if [ -n "$$missing" ]; then echo "Arquivos sem cabecalho MPL:"; echo "$$missing"; exit 1; fi
	@echo "OK: cabecalhos MPL encontrados."

test: all
	python3 -m unittest -v test_pipeline.py
	$(CXX) $(CXXFLAGS) -o test_spsc test_spsc.cpp -pthread
	./test_spsc
	rm -f test_spsc

clean:
	rm -f libmotor_grafico.so libmotor_grafico.dylib motor_grafico.dll test_spsc
