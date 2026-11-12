// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "processamento.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>

int main() {
    constexpr std::uint64_t TOTAL = 100000;
    FilaPipeline fila;
    std::atomic<bool> falhou{false};

    std::thread produtor([&] {
        for (std::uint64_t i = 1; i <= TOTAL; ++i) {
            DadosCena item;
            item.idMoldura = i;
            while (!fila.empurrar(std::move(item))) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumidor([&] {
        for (std::uint64_t esperado = 1; esperado <= TOTAL; ++esperado) {
            DadosCena item;
            while (!fila.puxar(item)) {
                std::this_thread::yield();
            }
            if (item.idMoldura != esperado) {
                falhou.store(true, std::memory_order_relaxed);
                return;
            }
        }
    });

    produtor.join();
    consumidor.join();
    assert(!falhou.load(std::memory_order_relaxed));
    std::cout << "SPSC OK: " << TOTAL << " itens em ordem.\n";
    return 0;
}
