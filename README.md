High-Performance Pipeline

A high-performance asynchronous pipeline, focused on enhancing parallel processing between the GPU and CPU.

 Technologies Used
Modern C++ (Advanced use of threads and lock-free atomic operations)
OpenCL (Hardware-accelerated computing for adaptive Ray Tracing)

 Features
Producer-Consumer Architecture: Dedicated CPU threads calculate geometry in parallel with rendering.
Smart Fallback: Automatic hardware detection (runs on Nvidia/AMD/Intel GPUs or activates high-performance CPU mode).
Safe Memory Management: Fast batch data transfer to VRAM/RAM without memory leaks.

 Licensed under the Mozilla Public License 2.0 by @NexCore (2026).
## Disclaimer

This software is provided "as is", without warranty of any kind, express or implied. In no event shall the author be liable for any claim, damages, data loss, or hardware failure arising from the use of this code. The use of this software is at your own risk.

## Partnerships and Commercial Licensing

If your company wishes to integrate this pipeline into a proprietary product without the obligation to open-source your modifications under the MPL 2.0 license, please contact the author to negotiate a private commercial license (Dual Licensing).

## How to Cite This Project

If you use this CPU/GPU pipeline in academic research, articles, or derivative projects, please attribute credit by citing this repository:
`Humberto5311 - CPU and GPU Pipeline (GitHub)`


# CPUandGPUpipeline

![version](https://img.shields.io/badge/version-2.2.0-blue)
![license](https://img.shields.io/badge/license-MPL--2.0-green)

Pipeline experimental de processamento vetorial com C++17, fila SPSC Producer-Consumer, worker thread, OpenCL opcional, fallback CPU nativo e modo hibrido CPU+GPU para lotes grandes. Nao e um renderizador nem implementa Ray Tracing.

## Arquitetura 2.2.0

1. A API C recebe um vetor de `float` e um fator de escala.
2. A thread chamadora publica uma tarefa na fila circular SPSC.
3. Uma thread consumidora retira a tarefa e executa o kernel OpenCL, quando disponivel.
4. Se houver GPU OpenCL e o lote tiver pelo menos 8.192 valores, a tarefa e dividida: CPU nativa e GPU processam partes diferentes em paralelo.
5. Para lotes menores, usa OpenCL; se OpenCL nao estiver compilado, nao houver dispositivo ou a execucao falhar, usa CPU nativa.
6. O resultado e reunido e devolvido ao chamador.

A fila e estritamente SPSC. A API serializa as chamadas de produtores externos para preservar esse contrato.

## Requisitos

- C++17 (GCC, Clang ou MinGW recente)
- GNU Make no Linux/macOS (opcional no Windows)
- Python 3.8+ para interface e testes
- OpenCL SDK/runtime opcional para aceleracao via OpenCL

Sem OpenCL, o projeto continua compilando e executa pelo fallback CPU nativo.

## Compilar

Linux/macOS:

```sh
make info
make
make test
```

O Makefile detecta OpenCL via `pkg-config`. Para compilar sem OpenCL, basta nao ter o pacote de desenvolvimento OpenCL instalado.

Windows (MinGW):

```bat
build.bat
python main.py
```

Para habilitar OpenCL no Windows, configure `OPENCL_ROOT` para o diretorio do SDK, contendo `include` e `lib`.

## Executar

```sh
python main.py
python main.py --scale 3 1 2 3 4
```

O exemplo multiplica cada valor pelo fator escolhido. O backend ativo e impresso no terminal.

## API C

- `bool ligarMotorGrafico()` — inicia o worker.
- `int processarGeometria(const float* entrada, size_t tamanho, float* saida, float escala)` — processa o vetor. Retorno `0` indica sucesso; valores negativos indicam entrada invalida, escala nao finita, falha ao iniciar ou interrupcao.
- `const char* obterBackend()` — retorna `CPU` ou `OpenCL`.
- `const char* obterVersao()` — retorna a versao compilada.
- `void desligarMotorGrafico()` — encerra o worker.
- `enviarGeometria(...)` — funcao de compatibilidade da API 2.0.x; processa com escala 1 e descarta o resultado.

O limite defensivo atual e de 10 milhoes de elementos por chamada. A memoria de entrada e saida deve ser valida durante a chamada.

## Testes

`make test` compila a biblioteca e executa testes Python contra a API real. Em CI, o workflow instala PoCL para exercitar o caminho OpenCL em CPU quando o runner oferece o runtime.

## Versoes

A versao oficial fica em `VERSION`. O historico esta em `CHANGELOG.md`.

## Licenca

Mozilla Public License 2.0. Consulte `LICENSE`.


## v2.2.0 — estabilidade e concorrência

Esta versão corrige a publicação concorrente do backend, garante a finalização
da thread auxiliar no modo híbrido e impede que exceções C++ atravessem a API C.
Os nomes de backend retornados por `obterBackend()` são `CPU`, `GPU`,
`OPENCL_CPU` ou `HYBRID`.

### Limites conhecidos
A chamada `processarGeometria()` permanece síncrona. A submissão é serializada
para manter seguro o contrato de produtor único da fila SPSC. Uma API realmente
assíncrona, com múltiplas tarefas pendentes e coleta por identificador, fica
reservada para uma versão futura.

### Testes
Execute `make test`. Para testar o caminho GPU de verdade, compile com OpenCL
disponível e confirme que `obterBackend()` retorna `GPU` ou `HYBRID`.
Um teste que retorna `CPU` valida apenas o fallback, não o kernel OpenCL.


## API assincrona (2.2.0)

A biblioteca agora oferece submissao e coleta por identificador:

- `submeterGeometria(entrada, tamanho, escala)` copia os dados e retorna um ID.
- `coletarGeometria(id, saida, capacidade, &tamanho_saida, timeout_ms)` aguarda
  o resultado e o copia para o buffer fornecido.
- `processarGeometria(...)` permanece como atalho sincrono compativel.

A API aceita ate 64 tarefas pendentes por vez. Ao atingir o limite, uma nova
submissao retorna ID `0` imediatamente; colete resultados e tente novamente.
O limite evita crescimento ilimitado de memoria. A fila interna continua SPSC:
as chamadas de submissao sao serializadas, mas o chamador pode deixar varias
tarefas em andamento antes de coletar seus resultados.

Em Python, use `PipelineSession.submit()` e `PipelineSession.collect(task_id)`.
