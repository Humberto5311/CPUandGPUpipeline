// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "processamento.h"

#include <algorithm>
#include <condition_variable>
#include <cmath>
#include <cstdint>
#include <string>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
#include <exception>
#include <new>
#include <map>
#include <chrono>

#ifdef PIPELINE_WITH_OPENCL
  #define CL_TARGET_OPENCL_VERSION 120
  #ifdef __APPLE__
    #include <OpenCL/opencl.h>
  #else
    #include <CL/cl.h>
  #endif
#endif

namespace {
constexpr std::size_t MAX_ELEMENTOS = 10'000'000;

#ifdef PIPELINE_WITH_OPENCL
const char* KERNEL_FONTE = R"CLC(
__kernel void escalar_vetor(__global const float* entrada,
                            __global float* saida,
                            const float escala,
                            const ulong quantidade) {
    const size_t i = get_global_id(0);
    if ((ulong)i < quantidade) saida[i] = entrada[i] * escala;
}
)CLC";

class OpenCLMotor {
public:
    ~OpenCLMotor() { limpar(); }

    bool iniciar() {
        if (contexto_) return true;
        cl_uint total = 0;
        cl_int erro = clGetPlatformIDs(0, nullptr, &total);
        if (erro != CL_SUCCESS || total == 0) return false;
        std::vector<cl_platform_id> plataformas(total);
        if (clGetPlatformIDs(total, plataformas.data(), nullptr) != CL_SUCCESS) return false;

        // Prioridade: GPU dedicada/integrada; depois dispositivo OpenCL CPU.
        for (cl_device_type tipo : {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU,
                                    CL_DEVICE_TYPE_DEFAULT}) {
            bool encontrado = false;
            for (auto plataforma : plataformas) {
                cl_device_id candidato = nullptr;
                if (clGetDeviceIDs(plataforma, tipo, 1, &candidato, nullptr) == CL_SUCCESS) {
                    dispositivo_ = candidato;
                    encontrado = true;
                    break;
                }
            }
            if (encontrado) break;
        }
        if (!dispositivo_) return false;

        contexto_ = clCreateContext(nullptr, 1, &dispositivo_, nullptr, nullptr, &erro);
        if (erro != CL_SUCCESS || !contexto_) { limpar(); return false; }
        fila_ = clCreateCommandQueue(contexto_, dispositivo_, 0, &erro);
        if (erro != CL_SUCCESS || !fila_) { limpar(); return false; }
        const std::size_t comprimento = std::strlen(KERNEL_FONTE);
        programa_ = clCreateProgramWithSource(contexto_, 1, &KERNEL_FONTE,
                                               &comprimento, &erro);
        if (erro != CL_SUCCESS || !programa_) { limpar(); return false; }
        erro = clBuildProgram(programa_, 1, &dispositivo_, "", nullptr, nullptr);
        if (erro != CL_SUCCESS) { limpar(); return false; }
        kernel_ = clCreateKernel(programa_, "escalar_vetor", &erro);
        if (erro != CL_SUCCESS || !kernel_) { limpar(); return false; }

        char nome[256] = {};
        clGetDeviceInfo(dispositivo_, CL_DEVICE_NAME, sizeof(nome) - 1, nome, nullptr);
        nome_ = nome[0] ? nome : "OpenCL device";
        cl_device_type tipo_dispositivo = 0;
        clGetDeviceInfo(dispositivo_, CL_DEVICE_TYPE, sizeof(tipo_dispositivo),
                        &tipo_dispositivo, nullptr);
        gpu_ = (tipo_dispositivo & CL_DEVICE_TYPE_GPU) != 0;
        return true;
    }

    bool processar(const std::vector<float>& entrada, float escala,
                   std::vector<float>& saida) {
        if (!kernel_ || entrada.empty()) { saida = entrada; return kernel_ != nullptr; }
        const std::size_t bytes = entrada.size() * sizeof(float);
        cl_int erro = CL_SUCCESS;
        cl_mem in = clCreateBuffer(contexto_, CL_MEM_READ_ONLY, bytes, nullptr, &erro);
        if (erro != CL_SUCCESS || !in) return false;
        cl_mem out = clCreateBuffer(contexto_, CL_MEM_WRITE_ONLY, bytes, nullptr, &erro);
        if (erro != CL_SUCCESS || !out) { clReleaseMemObject(in); return false; }

        const cl_ulong quantidade = static_cast<cl_ulong>(entrada.size());
        erro = clEnqueueWriteBuffer(fila_, in, CL_TRUE, 0, bytes, entrada.data(), 0, nullptr, nullptr);
        if (erro == CL_SUCCESS) erro = clSetKernelArg(kernel_, 0, sizeof(in), &in);
        if (erro == CL_SUCCESS) erro = clSetKernelArg(kernel_, 1, sizeof(out), &out);
        if (erro == CL_SUCCESS) erro = clSetKernelArg(kernel_, 2, sizeof(escala), &escala);
        if (erro == CL_SUCCESS) erro = clSetKernelArg(kernel_, 3, sizeof(quantidade), &quantidade);
        const std::size_t global = entrada.size();
        if (erro == CL_SUCCESS)
            erro = clEnqueueNDRangeKernel(fila_, kernel_, 1, nullptr, &global, nullptr,
                                          0, nullptr, nullptr);
        saida.resize(entrada.size());
        if (erro == CL_SUCCESS)
            erro = clEnqueueReadBuffer(fila_, out, CL_TRUE, 0, bytes, saida.data(),
                                       0, nullptr, nullptr);
        clReleaseMemObject(out);
        clReleaseMemObject(in);
        return erro == CL_SUCCESS;
    }

    const std::string& nome() const { return nome_; }
    bool ehGPU() const { return gpu_; }

private:
    void limpar() {
        if (kernel_) { clReleaseKernel(kernel_); kernel_ = nullptr; }
        if (programa_) { clReleaseProgram(programa_); programa_ = nullptr; }
        if (fila_) { clReleaseCommandQueue(fila_); fila_ = nullptr; }
        if (contexto_) { clReleaseContext(contexto_); contexto_ = nullptr; }
        dispositivo_ = nullptr;
    }
    cl_device_id dispositivo_ = nullptr;
    cl_context contexto_ = nullptr;
    cl_command_queue fila_ = nullptr;
    cl_program programa_ = nullptr;
    cl_kernel kernel_ = nullptr;
    std::string nome_ = "OpenCL";
    bool gpu_ = false;
};
#endif

enum class BackendState : std::uint8_t {
    CPU,
    OPENCL_GPU,
    OPENCL_CPU,
    HYBRID
};

// Garante que a thread auxiliar seja sempre aguardada, inclusive durante
// o desenrolamento de pilha causado por uma excecao.
class ThreadJoinGuard {
public:
    explicit ThreadJoinGuard(std::thread& thread) noexcept : thread_(thread) {}
    ThreadJoinGuard(const ThreadJoinGuard&) = delete;
    ThreadJoinGuard& operator=(const ThreadJoinGuard&) = delete;
    ~ThreadJoinGuard() {
        if (thread_.joinable()) {
            try { thread_.join(); } catch (...) { std::terminate(); }
        }
    }
    void join() {
        if (thread_.joinable()) thread_.join();
    }
private:
    std::thread& thread_;
};

class Pipeline {
    static constexpr std::size_t MAX_TAREFAS_PENDENTES = 64;
public:
    ~Pipeline() { parar(); }

    bool iniciar() {
        std::lock_guard<std::mutex> lock(submissao_);
        if (ativo_) return true;
        return iniciar_sem_lock();
    }

    std::uint64_t submeter(const float* entrada, std::size_t tamanho, float escala) {
        if (!entrada || tamanho == 0 || tamanho > MAX_ELEMENTOS ||
            !std::isfinite(escala)) return 0;

        std::lock_guard<std::mutex> producer_lock(submissao_);
        if (!ativo_ && !iniciar_sem_lock()) return 0;

        DadosCena tarefa;
        tarefa.idMoldura = ++sequencia_;
        tarefa.escala = escala;
        tarefa.posicoesObjetos.assign(entrada, entrada + tamanho);
        const auto id = tarefa.idMoldura;

        std::unique_lock<std::mutex> lock(mutex_);
        // Limite explicito: nunca bloquear indefinidamente o chamador
        // quando ele submete tarefas sem coletar resultados anteriores.
        if (tarefas_pendentes_ >= MAX_TAREFAS_PENDENTES || parar_worker_) return 0;

        while (!fila_.empurrar(std::move(tarefa))) {
            cv_espaco_.wait(lock, [&] {
                const auto head = fila_.head.load(std::memory_order_acquire);
                const auto tail = fila_.tail.load(std::memory_order_acquire);
                return parar_worker_ || (tail - head < TAMANHO_FILA);
            });
            if (parar_worker_) return 0;
        }
        ++tarefas_pendentes_;
        lock.unlock();
        cv_trabalho_.notify_one();
        return id;
    }

    int coletar(std::uint64_t id, float* saida, std::size_t capacidade,
                std::size_t* tamanho_saida, int timeout_ms) {
        if (id == 0 || !tamanho_saida || timeout_ms < 0) return -1;
        std::unique_lock<std::mutex> lock(mutex_);
        const bool pronto = cv_conclusao_.wait_for(
            lock, std::chrono::milliseconds(timeout_ms), [&] {
                return resultados_.find(id) != resultados_.end() || erro_id_ == id;
            });
        if (!pronto) return -4;

        if (erro_id_ == id) {
            erro_id_ = 0;
            --tarefas_pendentes_;
            return -6;
        }

        auto it = resultados_.find(id);
        *tamanho_saida = it->second.size();
        if (!saida || capacidade < it->second.size()) return -5;
        std::copy(it->second.begin(), it->second.end(), saida);
        resultados_.erase(it);
        --tarefas_pendentes_;
        return 0;
    }

    int processar(const float* entrada, std::size_t tamanho, float* saida, float escala) {
        if (!entrada || !saida || tamanho == 0 || tamanho > MAX_ELEMENTOS) return -1;
        if (!std::isfinite(escala)) return -2;
        const auto id = submeter(entrada, tamanho, escala);
        if (id == 0) return -3;
        std::size_t produzido = 0;
        const int status = coletar(id, saida, tamanho, &produzido, 30000);
        if (status != 0) return status;
        return produzido == tamanho ? 0 : -5;
    }

    void parar() {
        std::lock_guard<std::mutex> producer_lock(submissao_);
        if (!ativo_) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            parar_worker_ = true;
        }
        cv_trabalho_.notify_all();
        cv_conclusao_.notify_all();
        cv_espaco_.notify_all();
        if (worker_.joinable()) worker_.join();
        ativo_ = false;
#ifdef PIPELINE_WITH_OPENCL
        opencl_pronto_ = false;
#endif
        backend_state_.store(BackendState::CPU, std::memory_order_release);
    }

    const char* backend() const noexcept {
        switch (backend_state_.load(std::memory_order_acquire)) {
            case BackendState::OPENCL_GPU: return "GPU";
            case BackendState::OPENCL_CPU: return "OPENCL_CPU";
            case BackendState::HYBRID: return "HYBRID";
            case BackendState::CPU:
            default: return "CPU";
        }
    }

private:
    bool iniciar_sem_lock() {
#ifdef PIPELINE_WITH_OPENCL
        opencl_pronto_ = opencl_.iniciar();
        if (opencl_pronto_) {
            backend_state_.store(opencl_.ehGPU() ? BackendState::OPENCL_GPU
                                                 : BackendState::OPENCL_CPU,
                                  std::memory_order_release);
            std::cout << "[Pipeline] Dispositivo: " << opencl_.nome() << '\n';
        } else {
            backend_state_.store(BackendState::CPU, std::memory_order_release);
            std::cout << "[Pipeline] OpenCL indisponivel; usando CPU nativa.\n";
        }
#else
        backend_state_.store(BackendState::CPU, std::memory_order_release);
        std::cout << "[Pipeline] Compilado sem OpenCL; usando CPU nativa.\n";
#endif
        {
            std::lock_guard<std::mutex> lock(mutex_);
            parar_worker_ = false;
        }
        try {
            worker_ = std::thread(&Pipeline::consumir, this);
        } catch (...) {
            backend_state_.store(BackendState::CPU, std::memory_order_release);
            return false;
        }
        ativo_ = true;
        return true;
    }

    void consumir() noexcept {
        for (;;) {
            DadosCena tarefa;
            if (fila_.puxar(tarefa)) {
                cv_espaco_.notify_one();
                std::vector<float> calculado;
                bool ok = false;
                BackendState backend_usado = BackendState::CPU;
                try {
#ifdef PIPELINE_WITH_OPENCL
                    if (opencl_pronto_ && opencl_.ehGPU() &&
                        tarefa.posicoesObjetos.size() >= 8192) {
                        const auto meio = tarefa.posicoesObjetos.size() / 2;
                        std::vector<float> parte_gpu(
                            tarefa.posicoesObjetos.begin() + static_cast<std::ptrdiff_t>(meio),
                            tarefa.posicoesObjetos.end());
                        std::vector<float> resultado_gpu;
                        calculado.resize(tarefa.posicoesObjetos.size());

                        std::thread trabalho_cpu([&] {
                            std::transform(
                                tarefa.posicoesObjetos.begin(),
                                tarefa.posicoesObjetos.begin() + static_cast<std::ptrdiff_t>(meio),
                                calculado.begin(),
                                [&](float v) { return v * tarefa.escala; });
                        });
                        ThreadJoinGuard join_cpu(trabalho_cpu);

                        // Se OpenCL lancar uma excecao, o guard aguarda a thread
                        // CPU antes de o buffer compartilhado sair de escopo.
                        const bool gpu_ok = opencl_.processar(
                            parte_gpu, tarefa.escala, resultado_gpu);
                        join_cpu.join();

                        if (gpu_ok && resultado_gpu.size() == tarefa.posicoesObjetos.size() - meio) {
                            std::copy(resultado_gpu.begin(), resultado_gpu.end(),
                                      calculado.begin() + static_cast<std::ptrdiff_t>(meio));
                            ok = true;
                            backend_usado = BackendState::HYBRID;
                        }
                    } else if (opencl_pronto_) {
                        ok = opencl_.processar(tarefa.posicoesObjetos,
                                               tarefa.escala, calculado);
                        if (ok) {
                            backend_usado = opencl_.ehGPU()
                                ? BackendState::OPENCL_GPU : BackendState::OPENCL_CPU;
                        }
                    }
#endif
                } catch (...) {
                    // A tarefa será refeita integralmente na CPU abaixo.
                    ok = false;
                }

                if (!ok) {
                    try {
                        calculado.resize(tarefa.posicoesObjetos.size());
                        std::transform(tarefa.posicoesObjetos.begin(),
                                       tarefa.posicoesObjetos.end(), calculado.begin(),
                                       [&](float v) { return v * tarefa.escala; });
                        backend_usado = BackendState::CPU;
                    } catch (...) {
                        // Falha irrecuperável de memória: publica conclusão vazia.
                        calculado.clear();
                        backend_usado = BackendState::CPU;
                    }
                }

                backend_state_.store(backend_usado, std::memory_order_release);
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    try {
                        resultados_.emplace(tarefa.idMoldura, std::move(calculado));
                    } catch (...) {
                        erro_id_ = tarefa.idMoldura;
                    }
                }
                cv_conclusao_.notify_all();
                continue;
            }

            std::unique_lock<std::mutex> lock(mutex_);
            cv_trabalho_.wait(lock, [&] {
                return parar_worker_ ||
                    fila_.head.load(std::memory_order_acquire) !=
                    fila_.tail.load(std::memory_order_acquire);
            });
            if (parar_worker_ &&
                fila_.head.load(std::memory_order_acquire) ==
                fila_.tail.load(std::memory_order_acquire)) {
                break;
            }
        }
    }

    FilaPipeline fila_;
    std::thread worker_;
    std::mutex submissao_;
    std::mutex mutex_;
    std::condition_variable cv_trabalho_;
    std::condition_variable cv_conclusao_;
    std::condition_variable cv_espaco_;
    bool ativo_ = false;
    bool parar_worker_ = false;
    unsigned long long sequencia_ = 0;
    std::size_t tarefas_pendentes_ = 0;
    std::uint64_t erro_id_ = 0;
    std::map<std::uint64_t, std::vector<float>> resultados_;
    std::atomic<BackendState> backend_state_{BackendState::CPU};
#ifdef PIPELINE_WITH_OPENCL
    OpenCLMotor opencl_;
    bool opencl_pronto_ = false;
#endif
};

Pipeline motor;
} // namespace

extern "C" {
PIPELINE_API bool ligarMotorGrafico() {
    try { return motor.iniciar(); } catch (...) { return false; }
}

PIPELINE_API int processarGeometria(const float* entrada, std::size_t tamanho,
                                    float* saida, float escala) {
    try { return motor.processar(entrada, tamanho, saida, escala); }
    catch (const std::bad_alloc&) { return -6; }
    catch (...) { return -7; }
}

PIPELINE_API std::uint64_t submeterGeometria(const float* entrada, std::size_t tamanho,
                                                   float escala) {
    try { return motor.submeter(entrada, tamanho, escala); }
    catch (...) { return 0; }
}

PIPELINE_API int coletarGeometria(std::uint64_t id, float* saida, std::size_t capacidade,
                                  std::size_t* tamanho_saida, int timeout_ms) {
    try { return motor.coletar(id, saida, capacidade, tamanho_saida, timeout_ms); }
    catch (...) { return -7; }
}

PIPELINE_API void enviarGeometria(const float* arrayPosicoes, int tamanho) {
    if (!arrayPosicoes || tamanho <= 0) return;
    std::vector<float> resultado(static_cast<std::size_t>(tamanho));
    (void)motor.processar(arrayPosicoes, static_cast<std::size_t>(tamanho),
                          resultado.data(), 1.0f);
}

PIPELINE_API const char* obterBackend() { return motor.backend(); }
PIPELINE_API const char* obterVersao() { return APP_VERSION; }
PIPELINE_API void desligarMotorGrafico() {
    try { motor.parar(); } catch (...) { /* nunca propagar excecao pela ABI C */ }
}
}
