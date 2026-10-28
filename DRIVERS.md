# Release 2.0.0

## Novidades
- **Fila circular com 8 espacos** (antes era 1): a CPU nao precisa mais esperar a GPU esvaziar cada quadro.
- **Ordem de memoria correta** (`acquire`/`release`): a GPU so le o quadro depois que a CPU terminou de escrever.
- **`head` e `tail` separados em 64 bytes** (`alignas(64)`): evita uma thread atrapalhar a outra (false sharing).
- **`ContextoOpenCL` (RAII)**: limpa o OpenCL sozinho, sem precisar lembrar de chamar `limparOpenCL()`.
- **Teste automatico** (`test_fila.cpp`): 1.000.000 de quadros, conferindo ordem e conteudo.
- **`processamento.cpp` atualizado**:
  - Removidas as definicoes antigas da fila (agora no `.h`).
  - O buffer da GPU e **reaproveitado** entre quadros (antes era criado e destruido a cada quadro).
  - Erros do OpenCL sao verificados (antes `clCreateContext` e `clCreateCommandQueue` podiam falhar e a funcao dizia "true").
  - Corrigido risco de ler alem do array quando havia mais de 4 plataformas OpenCL.
  - `ligarMotorGrafico` 2x nao vaza memoria; `enviarGeometria` antes de ligar nao trava; entradas invalidas (nulo, tamanho <= 0) sao ignoradas.
  - `limparOpenCL` zera os ponteiros (pode ser chamada 2x).
  - `__declspec(dllexport)` virou macro `EXPORTAR` (funciona no Windows e no Linux).
- **`main.py` atualizado**:
  - **Removido o pedido de senha de Admin/Root.** O programa nao usa a senha e nao precisa de privilegios elevados.
  - Removidas as mensagens falsas de "aplicando permissoes de root".
  - O sistema operacional e detectado sozinho (Windows, macOS e Linux), sem perguntar.
  - Nao depende mais do `questionary` (menos coisa para instalar).
  - A biblioteca e procurada ao lado do `main.py`, e nao na pasta de onde o programa foi aberto.
  - Tipos do `ctypes` declarados; o motor e sempre desligado no final (`finally`).
- Licenca: link correto da MPL 2.0 e `#include <cstddef>`.

- **Build facil**: `Makefile` (Linux/Windows/macOS), `build.bat` (Windows), `.gitignore` e `DRIVERS.md`.
- **Build automatico no GitHub** (`.github/workflows/build.yml`): gera a `.so`, a `.dll` e o `.exe`.

## Compatibilidade
- Substitua **os dois arquivos**: `processamento.h` e `processamento.cpp`.
- `enviarGeometria` agora recebe `const float*` (continua compativel com quem chama).
- `empurrarParaGPU` e `puxarParaGPU` continuam existindo (chamam `empurrar` e `puxar`).
- `inicializarOpenCLComercial`, `transferirDadosParaHardware` e `limparOpenCL` nao mudaram.

## Como gerar os binarios
- Linux: `make`  ->  `libmotor_grafico.so`
- Windows: `build.bat exe`  ->  `motor_grafico.dll` e `dist\CPUandGPUpipeline.exe`
- Ou deixe o GitHub gerar (aba Actions). Anexe os arquivos na Release; nao os coloque no codigo.

## Como testar
```
g++ -std=c++17 -O2 -pthread test_fila.cpp -o test_fila
./test_fila
```

## Aviso
A fila so e segura com exatamente **1 thread produtora e 1 consumidora**.

## Ainda nao feito (ideias para a 2.1)
- `enviarGeometria` ainda envia direto para o hardware; a `FilaPipeline` ainda nao esta ligada a uma thread consumidora no motor.
- Nenhum kernel de Ray Tracing roda ainda: os dados chegam ao buffer, mas nao ha `clEnqueueNDRangeKernel`.
- Testado com um OpenCL simulado (sem GPU real).
