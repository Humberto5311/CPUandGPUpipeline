# Política de Segurança / Security Policy

## Versões Suportadas / Supported Versions

| Versão / Version | Suportada / Supported |
| ---------------- | --------------------- |
| 2.0.x            | Sim / Yes             |
| < 2.0            | Não / No              |

## Escopo / Scope

Relatos relevantes incluem: corrupção de memória, data races nas filas
produtor-consumidor, acessos fora dos limites em kernels OpenCL, falhas na
transferência de dados CPU/GPU e carregamento inseguro da biblioteca dinâmica.

In scope: memory corruption, data races in the producer-consumer queues,
out-of-bounds access in OpenCL kernels, CPU/GPU transfer flaws and unsafe
loading of the dynamic library.

## Como relatar uma vulnerabilidade / Reporting a Vulnerability

**Não abra uma Issue pública.** Use a aba **Security → Report a vulnerability**
deste repositório (relatório privado do GitHub).

**Please do not open a public Issue.** Use **Security → Report a vulnerability**
on this repository (GitHub private vulnerability reporting).

Inclua / Please include: versão afetada, passos para reproduzir, sistema
operacional, hardware/driver (Nvidia/AMD/Intel) e impacto estimado.

## Prazos / Timeline

- Confirmação / Acknowledgement: até 7 dias / within 7 days
- Plano de correção / Fix plan: até 30 dias / within 30 days

Daremos crédito a quem reportar, se desejar, e pedimos que a divulgação pública
ocorra somente após a correção. / Reporters will be credited if they wish, and we
ask that public disclosure happens only after a fix is released.
