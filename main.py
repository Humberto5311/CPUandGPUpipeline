# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import ctypes
import platform
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
MAX_ELEMENTS = 10_000_000


def library_name():
    system = platform.system()
    if system == "Windows":
        return "motor_grafico.dll"
    if system == "Darwin":
        return "libmotor_grafico.dylib"
    return "libmotor_grafico.so"


def load_engine():
    path = ROOT / library_name()
    if not path.is_file():
        raise FileNotFoundError(
            f"Biblioteca nao encontrada: {path.name}. Compile primeiro com make ou build.bat."
        )
    lib = ctypes.CDLL(str(path))
    float_ptr = ctypes.POINTER(ctypes.c_float)
    lib.ligarMotorGrafico.argtypes = []
    lib.ligarMotorGrafico.restype = ctypes.c_bool
    lib.processarGeometria.argtypes = [float_ptr, ctypes.c_size_t, float_ptr, ctypes.c_float]
    lib.processarGeometria.restype = ctypes.c_int
    lib.submeterGeometria.argtypes = [float_ptr, ctypes.c_size_t, ctypes.c_float]
    lib.submeterGeometria.restype = ctypes.c_uint64
    lib.coletarGeometria.argtypes = [ctypes.c_uint64, float_ptr, ctypes.c_size_t,
                                     ctypes.POINTER(ctypes.c_size_t), ctypes.c_int]
    lib.coletarGeometria.restype = ctypes.c_int
    lib.obterBackend.argtypes = []
    lib.obterBackend.restype = ctypes.c_char_p
    lib.obterVersao.argtypes = []
    lib.obterVersao.restype = ctypes.c_char_p
    lib.desligarMotorGrafico.argtypes = []
    lib.desligarMotorGrafico.restype = None
    return lib


class PipelineSession:
    """Sessao persistente com submissao assincrona e coleta por ID."""

    def __init__(self):
        self.lib = load_engine()
        if not self.lib.ligarMotorGrafico():
            raise RuntimeError("Nao foi possivel iniciar a pipeline.")
        self.pending = {}

    def submit(self, values, scale):
        if not values or len(values) > MAX_ELEMENTS:
            raise ValueError(f"Informe entre 1 e {MAX_ELEMENTS} valores.")
        input_array = (ctypes.c_float * len(values))(*values)
        task_id = self.lib.submeterGeometria(input_array, len(values), scale)
        if not task_id:
            raise RuntimeError("Falha ao submeter tarefa.")
        self.pending[task_id] = len(values)
        return task_id

    def collect(self, task_id, timeout_ms=30000):
        if task_id not in self.pending:
            raise ValueError("ID de tarefa desconhecido ou ja coletado.")
        count = self.pending[task_id]
        output_array = (ctypes.c_float * count)()
        output_size = ctypes.c_size_t()
        status = self.lib.coletarGeometria(
            task_id, output_array, count, ctypes.byref(output_size), timeout_ms
        )
        if status != 0:
            if status != -4:
                self.pending.pop(task_id, None)
            raise RuntimeError(f"Falha ao coletar tarefa (codigo {status}).")
        self.pending.pop(task_id, None)
        return list(output_array[:output_size.value])

    def backend(self):
        return self.lib.obterBackend().decode("utf-8", errors="replace")

    def version(self):
        return self.lib.obterVersao().decode("utf-8", errors="replace")

    def close(self):
        if self.lib is not None:
            # Consome resultados ainda pendentes para nao deixar a capacidade
            # limitada da sessao ocupada ao reutilizar a biblioteca.
            for task_id in list(self.pending):
                try:
                    self.collect(task_id)
                except (RuntimeError, ValueError):
                    self.pending.pop(task_id, None)
            self.lib.desligarMotorGrafico()
            self.lib = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()


def run(values, scale):
    with PipelineSession() as session:
        task_id = session.submit(values, scale)
        result = session.collect(task_id)
        return session.version(), session.backend(), result


def main():
    parser = argparse.ArgumentParser(description="Teste da pipeline CPU/GPU")
    parser.add_argument("--scale", type=float, default=2.0, help="Fator aplicado a cada valor")
    parser.add_argument("values", nargs="*", type=float,
                        help="Valores de entrada (padrao: 1 2 3 4 5 6)")
    args = parser.parse_args()
    values = args.values or [1.0, 2.0, 3.0, 4.0, 5.0, 6.0]
    version, backend, result = run(values, args.scale)
    print(f"CPUandGPUpipeline v{version}")
    print(f"Backend: {backend}")
    print(f"Entrada: {values}")
    print(f"Escala:  {args.scale}")
    print(f"Saida:   {result}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"[ERRO] {exc}", file=sys.stderr)
        sys.exit(1)
