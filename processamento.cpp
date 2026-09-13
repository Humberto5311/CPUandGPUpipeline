#include "processamento.h"
#include <iostream>
#include <CL/cl.h> 

static cl_platform_id plataforma;
static cl_device_id dispositivo_escolhido;
static cl_context contexto;
static cl_command_queue fila_comandos;

bool FilaPipeline::empurrarParaGPU(DadosCena&& dados) {
    bool esperado = false;
    while (!ocupado.compare_exchange_weak(esperado, true, std::memory_order_acquire, std::memory_order_relaxed)) {
        esperado = false; 
    }

    buffer = std::move(dados);
    tail.store(tail.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    ocupado.store(false, std::memory_order_release);
    return true;
}

bool FilaPipeline::puxarParaGPU(DadosCena& saida) {
    size_t atualHead = head.load(std::memory_order_relaxed);
    size_t atualTail = tail.load(std::memory_order_acquire);

    if (atualHead == atualTail) return false; 

    bool esperado = false;
    while (!ocupado.compare_exchange_weak(esperado, true, std::memory_order_acquire, std::memory_order_relaxed)) {
        esperado = false;
    }

    saida = std::move(buffer);
    head.store(atualHead + 1, std::memory_order_release);
    ocupado.store(false, std::memory_order_release); 
    return true;
}

bool inicializarOpenCLComercial() {
    cl_int erro;
    cl_uint numPlataformas;
    cl_platform_id plataformas[4]; 

    erro = clGetPlatformIDs(4, plataformas, &numPlataformas);
    if (erro != CL_SUCCESS || numPlataformas == 0) {
        std::cerr << "[ERROR] Nenhum driver OpenCL instalado no PC do cliente." << std::endl;
        return false;
    }

    bool achouHardware = false;

    for (cl_uint i = 0; i < numPlataformas; ++i) {
        erro = clGetDeviceIDs(plataformas[i], CL_DEVICE_TYPE_GPU, 1, &dispositivo_escolhido, NULL);
        if (erro == CL_SUCCESS) {
            achouHardware = true;
            break; 
        }
    }
   
    if (!achouHardware) {
        std::cout << " GPU nao detectada. Ativando Modo CPU de Alto Desempenho..." << std::endl;
        for (cl_uint i = 0; i < numPlataformas; ++i) {
            erro = clGetDeviceIDs(plataformas[i], CL_DEVICE_TYPE_CPU, 1, &dispositivo_escolhido, NULL);
            if (erro == CL_SUCCESS) {
                achouHardware = true;
                break;
            }
        }
    }

    if (!achouHardware) {
        std::cerr << " O hardware do cliente nao suporta computacao paralela." << std::endl;
        return false;
    }

    char nomeHardware[128];
    clGetDeviceInfo(dispositivo_escolhido, CL_DEVICE_NAME, sizeof(nomeHardware), nomeHardware, NULL);
    std::cout << " Rodando Ray Tracing em: " << nomeHardware << std::endl;

    contexto = clCreateContext(NULL, 1, &dispositivo_escolhido, NULL, NULL, &erro);
    fila_comandos = clCreateCommandQueue(contexto, dispositivo_escolhido, 0, &erro);

    return true;
}

void transferirDadosParaHardware(const DadosCena& dados) {
    cl_int erro;
    size_t tamanhoBytes = dados.posicoesObjetos.size() * sizeof(float);

    if (tamanhoBytes == 0) return;

    cl_mem bufferHardware = clCreateBuffer(contexto, CL_MEM_READ_ONLY, tamanhoBytes, NULL, &erro);
    clEnqueueWriteBuffer(fila_comandos, bufferHardware, CL_TRUE, 0, tamanhoBytes, dados.posicoesObjetos.data(), 0, NULL, NULL);
    clReleaseMemObject(bufferHardware);
}

void limparOpenCL() {
    if (fila_comandos) clReleaseCommandQueue(fila_comandos);
    if (contexto) clReleaseContext(contexto);
}


extern "C" {
    __declspec(dllexport) bool ligarMotorGrafico() {
        return inicializarOpenCLComercial();
    }

    __declspec(dllexport) void enviarGeometria(float* arrayPosicoes, int tamanho) {
        DadosCena novaCena;
        novaCena.idMoldura = 1;
        for(int i = 0; i < tamanho; i++) {
            novaCena.posicoesObjetos.push_back(arrayPosicoes[i]);
        }
        transferirDadosParaHardware(novaCena);
    }

    __declspec(dllexport) void desligarMotorGrafico() {
        limparOpenCL();
    }
}
