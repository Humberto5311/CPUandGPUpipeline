# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.
import ctypes
import platform
import sys
from pathlib import Path

RAIZ = Path(__file__).resolve().parent


def versao():
    arquivo = RAIZ / "VERSION"
    return arquivo.read_text().strip() if arquivo.exists() else "dev"


def nome_biblioteca():
    sistema = platform.system()
    if sistema == "Windows":
        return "motor_grafico.dll"
    if sistema == "Darwin":
        return "libmotor_grafico.dylib"
    return "libmotor_grafico.so"


def inicializar_motor_grafico():
    caminho = RAIZ / nome_biblioteca()
    if not caminho.exists():
        print(f"\n[AVISO] Arquivo binario nao encontrado: {caminho}")
        print("        Compile antes com 'make' (Linux/macOS) ou build.bat (Windows).")
        return None

    try:
        motor = ctypes.CDLL(str(caminho))
    except OSError as e:
        print(f"\n[ERRO] Nao foi possivel carregar a biblioteca do motor grafico: {e}")
        return None

    motor.ligarMotorGrafico.argtypes = []
    motor.ligarMotorGrafico.restype = ctypes.c_bool
    motor.enviarGeometria.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_int]
    motor.enviarGeometria.restype = None
    motor.desligarMotorGrafico.argtypes = []
    motor.desligarMotorGrafico.restype = None
    return motor


def run_pipeline():
    print("\n" + "=" * 50)
    print(f"   CPU and GPU Pipeline v{versao()}")
    print("=" * 50)
    print(f"-> Sistema detectado: {platform.system()}")

    motor = inicializar_motor_grafico()
    if motor is None:
        return 1

    if not motor.ligarMotorGrafico():
        print("\n[ERRO] O pipeline nao pode ser ativado: nenhum hardware OpenCL disponivel.")
        return 1

    try:
        dados_python = [0.0, 1.0, -5.0, 2.0, 1.5, -3.0]
        dados_para_c = (ctypes.c_float * len(dados_python))(*dados_python)
        motor.enviarGeometria(dados_para_c, len(dados_python))
    finally:
        motor.desligarMotorGrafico()

    print("\n[OK] Pipeline executado com sucesso.")
    print("=" * 50 + "\n")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(run_pipeline())
    except KeyboardInterrupt:
        print("\n\nProcesso interrompido.")
        sys.exit(130)
