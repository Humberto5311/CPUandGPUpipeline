# Contributing to CPU and GPU Pipeline (NexCore)

Thank you for your interest in improving this project!

## How to Contribute

1. **Fork** this repository.
2. Create a new branch for your feature or bugfix.
3. Build and check before committing:
   ```bash
   make
   make check-license
   ```
4. Commit your changes with clear messages.
5. Open a **Pull Request** explaining your modifications.

## Rules

- Every new `.cpp`, `.h` and `.py` file must start with the MPL 2.0 notice
  (see Exhibit A in [LICENSE](LICENSE)). `make check-license` verifies this.
- The queue in `processamento.h` is **SPSC** (1 producer, 1 consumer). Changes
  must not break this contract without documenting it.
- Do not report security problems in public issues. See [SECURITY.md](SECURITY.md).
- Do not add code that asks for passwords or elevated privileges.

## License of Contributions

Any contribution to this repository will be licensed under the Mozilla Public
License 2.0.

By submitting a Pull Request, you also agree that the maintainer may offer your
contribution under separate commercial terms (Dual Licensing), as described in
the README. If you do not agree, please do not submit a contribution.


## Verificacao da versao 2.2.0

Antes de enviar alteracoes, execute `make clean && make check-license && make test`.
O teste `test_spsc.cpp` exercita 100.000 itens entre um produtor e um consumidor.
Para validar o kernel OpenCL, use o job `linux-opencl` do GitHub Actions:
ele define `PIPELINE_REQUIRE_OPENCL=1` e falha se o backend cair no fallback CPU.
