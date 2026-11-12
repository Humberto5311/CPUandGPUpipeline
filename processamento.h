// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef PROCESSAMENTO_H
#define PROCESSAMENTO_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#ifndef APP_VERSION
#define APP_VERSION "0.0.0-dev"
#endif

struct DadosCena {
    std::uint64_t idMoldura = 0;
    float escala = 1.0f;
    std::vector<float> posicoesObjetos;
};

constexpr std::size_t TAMANHO_FILA = 8;
static_assert((TAMANHO_FILA & (TAMANHO_FILA - 1)) == 0,
              "TAMANHO_FILA deve ser potencia de 2");

// Fila SPSC: exatamente um produtor e um consumidor.
struct FilaPipeline {
    DadosCena slots[TAMANHO_FILA];
    alignas(64) std::atomic<std::size_t> head{0};
    alignas(64) std::atomic<std::size_t> tail{0};

    [[nodiscard]] bool empurrar(DadosCena&& dados) {
        const auto t = tail.load(std::memory_order_relaxed);
        if (t - head.load(std::memory_order_acquire) >= TAMANHO_FILA) return false;
        slots[t % TAMANHO_FILA] = std::move(dados);
        tail.store(t + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool puxar(DadosCena& saida) {
        const auto h = head.load(std::memory_order_relaxed);
        if (h == tail.load(std::memory_order_acquire)) return false;
        saida = std::move(slots[h % TAMANHO_FILA]);
        head.store(h + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empurrarParaGPU(DadosCena&& dados) {
        return empurrar(std::move(dados));
    }
    [[nodiscard]] bool puxarParaGPU(DadosCena& saida) { return puxar(saida); }
};

// API C exportada para Python, C, C++ e outras linguagens FFI.
#if defined(_WIN32)
  #if defined(PIPELINE_BUILD)
    #define PIPELINE_API __declspec(dllexport)
  #else
    #define PIPELINE_API __declspec(dllimport)
  #endif
#else
  #define PIPELINE_API __attribute__((visibility("default")))
#endif

extern "C" {
PIPELINE_API bool ligarMotorGrafico();
PIPELINE_API int processarGeometria(const float* entrada, std::size_t tamanho,
                                    float* saida, float escala);
PIPELINE_API std::uint64_t submeterGeometria(const float* entrada, std::size_t tamanho,
                                             float escala);
PIPELINE_API int coletarGeometria(std::uint64_t id, float* saida, std::size_t capacidade,
                                  std::size_t* tamanho_saida, int timeout_ms);
PIPELINE_API void enviarGeometria(const float* arrayPosicoes, int tamanho);
PIPELINE_API const char* obterBackend();
PIPELINE_API const char* obterVersao();
PIPELINE_API void desligarMotorGrafico();
}

#endif
