// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "processamento.h"
#include <algorithm>
#include <iostream>

#define CL_TARGET_OPENCL_VERSION 120
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#if defined(_WIN32)
#define PIPELINE_API __declspec(dllexport)
#else
#define PIPELINE_API __attribute__((visibility("default")))
#endif

static cl_device_id dispositivo_escolhido = nullptr;
static cl_context contexto = nullptr;
static cl_command_queue fila_comandos = nullptr;

static bool escolherDispositivo(const cl_platform_id* plataformas, cl_uint n,
                                cl_device_type tipo, cl_device_id& saida) {
    for (cl_uint i = 0; i < n; ++i) {
        cl_device_id d = nullptr;
        if (clGetDeviceIDs(plataformas[i], tipo, 1, &d, nullptr) == CL_SUCCESS) {
            saida = d;
            return true;
        }
    }
    return false;
}

bool inicializarOpenCLComercial() {
    if (contexto) return true; // ja inicializado

    cl_uint numPlataformas = 0;
    cl_platform_id plataformas[4];

    cl_int erro = clGetPlatformIDs(4, plataformas, &numPlataformas);
    if (erro != CL_SUCCESS || numPlataformas == 0) {
        std::cerr << "[ERROR] Nenhum driver OpenCL instalado no PC do cliente." << std::endl;
        return false;
    }
    numPlataformas = std::min<cl_uint>(numPlataformas, 4);

    cl_device_id dispositivo = nullptr;
    bool achouHardware = escolherDispositivo(plataformas, numPlataformas,
                                             CL_DEVICE_TYPE_GPU, dispositivo);
    if (!achouHardware) {
        std::cout << "GPU nao detectada. Ativando Modo CPU de Alto Desempenho..." << std::endl;
        achouHardware = escolherDispositivo(plataformas, numPlataformas,
                                            CL_DEVICE_TYPE_CPU, dispositivo);
    }
    if (!achouHardware) {
        std::cerr << "O hardware do cliente nao suporta computacao paralela." << std::endl;
        return false;
    }

    char nomeHardware[128] = {0};
    clGetDeviceInfo(dispositivo, CL_DEVICE_NAME, sizeof(nomeHardware) - 1, nomeHardware, nullptr);
    std::cout << "Rodando Ray Tracing em: " << nomeHardware << std::endl;

    cl_context ctx = clCreateContext(nullptr, 1, &dispositivo, nullptr, nullptr, &erro);
    if (erro != CL_SUCCESS || !ctx) {
        std::cerr << "[ERROR] Falha ao criar o contexto OpenCL (" << erro << ")." << std::endl;
        return false;
    }

    cl_command_queue fila = clCreateCommandQueue(ctx, dispositivo, 0, &erro);
    if (erro != CL_SUCCESS || !fila) {
        std::cerr << "[ERROR] Falha ao criar a fila de comandos (" << erro << ")." << std::endl;
        clReleaseContext(ctx);
        return false;
    }

    dispositivo_escolhido = dispositivo;
    contexto = ctx;
    fila_comandos = fila;
    return true;
}

void transferirDadosParaHardware(const DadosCena& dados) {
    if (!contexto || !fila_comandos) return; // sem OpenCL: modo CPU, nada a transferir

    const std::size_t tamanhoBytes = dados.posicoesObjetos.size() * sizeof(float);
    if (tamanhoBytes == 0) return;

    cl_int erro = CL_SUCCESS;
    cl_mem bufferHardware = clCreateBuffer(contexto, CL_MEM_READ_ONLY, tamanhoBytes, nullptr, &erro);
    if (erro != CL_SUCCESS || !bufferHardware) {
        std::cerr << "[ERROR] Falha ao alocar buffer no hardware (" << erro << ")." << std::endl;
        return;
    }

    erro = clEnqueueWriteBuffer(fila_comandos, bufferHardware, CL_TRUE, 0, tamanhoBytes,
                                dados.posicoesObjetos.data(), 0, nullptr, nullptr);
    if (erro != CL_SUCCESS) {
        std::cerr << "[ERROR] Falha ao transferir dados (" << erro << ")." << std::endl;
    }

    clReleaseMemObject(bufferHardware);
}

void limparOpenCL() {
    if (fila_comandos) { clReleaseCommandQueue(fila_comandos); fila_comandos = nullptr; }
    if (contexto) { clReleaseContext(contexto); contexto = nullptr; }
    dispositivo_escolhido = nullptr;
}

extern "C" {
    PIPELINE_API bool ligarMotorGrafico() {
        return inicializarOpenCLComercial();
    }

    PIPELINE_API void enviarGeometria(const float* arrayPosicoes, int tamanho) {
        if (!arrayPosicoes || tamanho <= 0) return;

        DadosCena novaCena;
        novaCena.idMoldura = 1;
        novaCena.posicoesObjetos.assign(arrayPosicoes, arrayPosicoes + tamanho);
        transferirDadosParaHardware(novaCena);
    }

    PIPELINE_API void desligarMotorGrafico() {
        limparOpenCL();
    }
}
