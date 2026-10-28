
Processamento · H
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
// CPUandGPUpipeline - Release 2.0.0
 
#ifndef PROCESSAMENTO_H
#define PROCESSAMENTO_H
 
#include <atomic>
#include <cstddef>
#include <utility>
#include <vector>
 
struct DadosCena {
    int idMoldura = 0;
    std::vector<float> posicoesObjetos;
};
 
// Quantidade de quadros que cabem na fila ao mesmo tempo.
constexpr std::size_t TAMANHO_FILA = 8;
static_assert(TAMANHO_FILA > 0, "A fila precisa ter pelo menos 1 espaco");
 
// Fila circular SPSC (1 thread produtora + 1 thread consumidora), sem travas.
// IMPORTANTE: so e segura com exatamente 1 produtor e 1 consumidor.
struct FilaPipeline {
    DadosCena slots[TAMANHO_FILA];
    alignas(64) std::atomic<std::size_t> head{0}; // lado da GPU (leitura)
    alignas(64) std::atomic<std::size_t> tail{0}; // lado da CPU (escrita)
 
    bool empurrar(DadosCena&& dados) {
        const std::size_t t = tail.load(std::memory_order_relaxed);
        if (t - head.load(std::memory_order_acquire) == TAMANHO_FILA) {
            return false;
        }
        slots[t % TAMANHO_FILA] = std::move(dados);
        tail.store(t + 1, std::memory_order_release);
        return true;
    }
 
    
    bool puxar(DadosCena& saida) {
        const std::size_t h = head.load(std::memory_order_relaxed);
        if (h == tail.load(std::memory_order_acquire)) {
            return false;
        }
        saida = std::move(slots[h % TAMANHO_FILA]);
        head.store(h + 1, std::memory_order_release);
        return true;
    }
 
    // Nomes antigos (Release 1), mantidos para nao quebrar codigo existente.
    bool empurrarParaGPU(DadosCena&& dados) { return empurrar(std::move(dados)); }
    bool puxarParaGPU(DadosCena& saida) { return puxar(saida); }
};
 
bool inicializarOpenCLComercial();
void transferirDadosParaHardware(const DadosCena& dados);
void limparOpenCL();
 
// RAII: inicializa o OpenCL ao criar e limpa sozinho ao destruir,
class ContextoOpenCL {
public:
    ContextoOpenCL() : ok_(inicializarOpenCLComercial()) {}
    ~ContextoOpenCL() { if (ok_) limparOpenCL(); }
 
    ContextoOpenCL(const ContextoOpenCL&) = delete;
    ContextoOpenCL& operator=(const ContextoOpenCL&) = delete;
 
    // true = GPU/OpenCL pronto; false = usar modo CPU (fallback).
    bool valido() const { return ok_; }
 
private:
    bool ok_;
};
 
#endif
