# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org.

#ifndef PROCESSAMENTO_H
#define PROCESSAMENTO_H

#include <vector>
#include <atomic> 

struct DadosCena {
    int idMoldura;
    std::vector<float> posicoesObjetos; 
};

struct FilaPipeline {
    DadosCena buffer;
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};
    std::atomic_bool ocupado{false}; 

    bool empurrarParaGPU(DadosCena&& dados);
    bool puxarParaGPU(DadosCena& saida);
};

// Funções de controle do motor gráfico
bool inicializarOpenCLComercial();
void transferirDadosParaHardware(const DadosCena& dados);
void limparOpenCL();

#endif
