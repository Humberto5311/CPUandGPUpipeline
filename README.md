# 🚀 Pipeline de Alto Desempenho

Um pipeline assíncrono de alta performance desenvolvido para engines de jogos, focado em otimizar o processamento paralelo entre GPU e CPU.

## 🛠️ Tecnologias Utilizadas
* **C++ Moderno** (Uso avançado de threads e operações atômicas lock-free)
* **OpenCL** (Computação acelerada por hardware para Ray Tracing adaptativo)

## 📌 Funcionalidades
* **Arquitetura Produtor-Consumidor:** Threads dedicadas na CPU calculam a geometria em paralelo com a renderização.
* **Fallback Inteligente:** Detecção automática de hardware (roda em GPUs Nvidia/AMD/Intel ou ativa o modo CPU de alto desempenho).
* **Memory Management Seguro:** Transferência rápida de dados em lote para a VRAM/RAM sem vazamentos de memória.

---
⚖️ Licenciado sob a **Mozilla Public License 2.0** por **NEXCore (2026)**.
