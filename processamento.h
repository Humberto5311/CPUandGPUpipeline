a#ifndef PROCESSAMENTO_H
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
