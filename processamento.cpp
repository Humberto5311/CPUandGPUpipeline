// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// CPUandGPUpipeline - Release 2.0.0

#define CL_TARGET_OPENCL_VERSION 120
#include "processamento.h"
#if defined(__APPLE__)
    #include <OpenCL/opencl.h>
#else
    #include <CL/cl.h>
#endif
#include <algorithm>
#include <atomic>
#include <iostream>

#if defined(_WIN32)
    #define EXPORTAR __declspec(dllexport)
#else
    #define EXPORTAR __attribute__((visibility("default")))
#endif

static cl_device_id dispositivo_escolhido = nullptr;
static cl_context contexto = nullptr;
static cl_command_queue fila_comandos = nullptr;
static cl_mem bufferHardware = nullptr;   // reaproveitado entre quadros
static std::size_t capacidadeBuffer = 0;

// Procura o primeiro dispositivo do tipo pedido (GPU ou CPU).
static bool procurarDispositivo(const cl_platform_id* plataformas, cl_uint n, cl_device_type tipo) {
    for (cl_uint i = 0; i < n; ++i) {
        if (clGetDeviceIDs(plataformas[i], tipo, 1, &dispositivo_escolhido, nullptr) == CL_SUCCESS) {
            return true;
        }
    }
    return false;
}

bool inicializarOpenCLComercial() {
    if (contexto) return true; // ja inicializado: nao cria de novo (evita vazamento)

    cl_int erro = CL_SUCCESS;
    cl_uint numPlataformas = 0;
    cl_platform_id plataformas[4];

    erro = clGetPlatformIDs(4, plataformas, &numPlataformas);
    if (erro != CL_SUCCESS || numPlataformas == 0) {
        std::cerr << "[ERROR] Nenhum driver OpenCL instalado no PC do cliente." << std::endl;
        return false;
    }
    // O numero devolvido pode ser maior que 4; so lemos o que cabe no array.
    numPlataformas = std::min<cl_uint>(numPlataformas, 4);

    bool achouHardware = procurarDispositivo(plataformas, numPlataformas, CL_DEVICE_TYPE_GPU);
    if (!achouHardware) {
        std::cout << " GPU nao detectada. Ativando Modo CPU de Alto Desempenho..." << std::endl;
        achouHardware = procurarDispositivo(plataformas, numPlataformas, CL_DEVICE_TYPE_CPU);
    }
    if (!achouHardware) {
        std::cerr << " O hardware do cliente nao suporta computacao paralela." << std::endl;
        return false;
    }

    char nomeHardware[128] = "desconhecido";
    clGetDeviceInfo(dispositivo_escolhido, CL_DEVICE_NAME, sizeof(nomeHardware), nomeHardware, nullptr);
    std::cout << " Rodando Ray Tracing em: " << nomeHardware << std::endl;

    contexto = clCreateContext(nullptr, 1, &dispositivo_escolhido, nullptr, nullptr, &erro);
    if (erro != CL_SUCCESS || !contexto) {
        std::cerr << "[ERROR] Falha ao criar o contexto OpenCL (codigo " << erro << ")." << std::endl;
        contexto = nullptr;
        dispositivo_escolhido = nullptr;
        return false;
    }

    fila_comandos = clCreateCommandQueue(contexto, dispositivo_escolhido, 0, &erro);
    if (erro != CL_SUCCESS || !fila_comandos) {
        std::cerr << "[ERROR] Falha ao criar a fila de comandos (codigo " << erro << ")." << std::endl;
        clReleaseContext(contexto);
        contexto = nullptr;
        fila_comandos = nullptr;
        dispositivo_escolhido = nullptr;
        return false;
    }

    return true;
}

void transferirDadosParaHardware(const DadosCena& dados) {
    if (!contexto || !fila_comandos) {
        std::cerr << "[ERROR] Motor grafico nao foi ligado (chame ligarMotorGrafico primeiro)." << std::endl;
        return;
    }

    const std::size_t tamanhoBytes = dados.posicoesObjetos.size() * sizeof(float);
    if (tamanhoBytes == 0) return;

    cl_int erro = CL_SUCCESS;

    // Release 2: o buffer so e recriado quando precisa crescer.
    // Antes, um buffer novo era criado e destruido a CADA quadro.
    if (tamanhoBytes > capacidadeBuffer) {
        if (bufferHardware) clReleaseMemObject(bufferHardware);
        const std::size_t novaCapacidade = std::max(tamanhoBytes, capacidadeBuffer * 2);
        bufferHardware = clCreateBuffer(contexto, CL_MEM_READ_ONLY, novaCapacidade, nullptr, &erro);
        if (erro != CL_SUCCESS || !bufferHardware) {
            std::cerr << "[ERROR] Falha ao criar buffer de " << novaCapacidade
                      << " bytes (codigo " << erro << ")." << std::endl;
            bufferHardware = nullptr;
            capacidadeBuffer = 0;
            return;
        }
        capacidadeBuffer = novaCapacidade;
    }

    // CL_TRUE = espera terminar, assim os dados podem ser liberados logo depois.
    erro = clEnqueueWriteBuffer(fila_comandos, bufferHardware, CL_TRUE, 0, tamanhoBytes,
                                dados.posicoesObjetos.data(), 0, nullptr, nullptr);
    if (erro != CL_SUCCESS) {
        std::cerr << "[ERROR] Falha ao enviar dados ao hardware (codigo " << erro << ")." << std::endl;
    }
}

void limparOpenCL() {
    if (bufferHardware) { clReleaseMemObject(bufferHardware); bufferHardware = nullptr; }
    capacidadeBuffer = 0;
    if (fila_comandos) { clReleaseCommandQueue(fila_comandos); fila_comandos = nullptr; }
    if (contexto) { clReleaseContext(contexto); contexto = nullptr; }
    dispositivo_escolhido = nullptr;
}

extern "C" {
    EXPORTAR bool ligarMotorGrafico() {
        return inicializarOpenCLComercial();
    }

    EXPORTAR void enviarGeometria(const float* arrayPosicoes, int tamanho) {
        if (!arrayPosicoes || tamanho <= 0) return;

        static std::atomic<int> contadorMoldura{0};
        DadosCena novaCena;
        novaCena.idMoldura = ++contadorMoldura;
        novaCena.posicoesObjetos.assign(arrayPosicoes, arrayPosicoes + tamanho);
        transferirDadosParaHardware(novaCena);
    }

    EXPORTAR void desligarMotorGrafico() {
        limparOpenCL();
    }
}
