# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# CPUandGPUpipeline - Release 2.0.0
# Este programa NAO pede senha e NAO precisa de privilegios de administrador.

import ctypes
import os
import platform
import sys


def caminho_biblioteca():
    """Descobre sozinho o nome da biblioteca de acordo com o sistema."""
    nomes = {
        "Windows": "motor_grafico.dll",
        "Darwin": "libmotor_grafico.dylib",  # macOS
    }
    nome = nomes.get(platform.system(), "libmotor_grafico.so")  # Linux
    # Procura ao lado deste arquivo (ou dentro do .exe gerado pelo PyInstaller),
    # nao importa de onde o programa foi aberto.
    pasta = getattr(sys, "_MEIPASS", None) or os.path.dirname(os.path.abspath(__file__))
    return os.path.join(pasta, nome)


def carregar_motor(caminho):
    motor = ctypes.CDLL(caminho)
    # Dizer os tipos evita erros estranhos na conversa Python <-> C++
    motor.ligarMotorGrafico.argtypes = []
    motor.ligarMotorGrafico.restype = ctypes.c_bool
    motor.enviarGeometria.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_int]
    motor.enviarGeometria.restype = None
    motor.desligarMotorGrafico.argtypes = []
    motor.desligarMotorGrafico.restype = None
    return motor


def main():
    print("=" * 50)
    print("        CPU and GPU Pipeline - Release 2.0.0")
    print("=" * 50)

    caminho = caminho_biblioteca()
    if not os.path.exists(caminho):
        print(f"\n[ERRO] Biblioteca nao encontrada: {caminho}")
        print("Compile o motor C++ primeiro: make (Linux) ou build.bat (Windows).")
        return 1

    try:
        motor = carregar_motor(caminho)
    except OSError as e:
        print(f"\n[ERRO] Nao consegui carregar a biblioteca: {e}")
        return 1

    if not motor.ligarMotorGrafico():
        print("\n[ERRO] Nao foi possivel ligar o motor (driver OpenCL ausente?).")
        return 1

    try:
        dados = [0.0, 1.0, -5.0, 2.0, 1.5, -3.0]
        array_c = (ctypes.c_float * len(dados))(*dados)
        motor.enviarGeometria(array_c, len(dados))
        print(f"\n[OK] {len(dados)} valores enviados ao hardware.")
    finally:
        motor.desligarMotorGrafico()  # sempre desliga, mesmo se der erro

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nInterrompido.")
        sys.exit(130)
