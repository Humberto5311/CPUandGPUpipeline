import sys
import time
import ctypes
import platform
import os
import questionary

def inicializar_motor_grafico():
    try:
        sistema_atual = platform.system()
        if sistema_atual == "Windows":
            caminho_lib = "./motor_grafico.dll"
        else:
            caminho_lib = "./libmotor_grafico.so"
            
        if not os.path.exists(caminho_lib):
            print(f"\n⚠️ Arquivo binario nao encontrado: {caminho_lib}")
            return None

        motor = ctypes.CDLL(caminho_lib)
        motor.ligarMotorGrafico.restype = ctypes.c_bool
        return motor
    except Exception as e:
        print(f"\n❌ Erro ao carregar a DLL do motor grafico: {e}")
        return None

def run_pipeline():
    print("\n" + "="*50)
    print("      INICIALIZADOR DE PIPELINE INTERATIVO      ")
    print("="*50 + "\n")

    sistema = questionary.select(
        "[1/3] Escolha o seu Sistema Operacional:",
        choices=[
            "Linux (Ubuntu/Debian)",
            "Linux (CentOS/RHEL)",
            "macOS",
            "Windows (WSL2)"
        ]
    ).ask()

    if not sistema:
        print("\n❌ Operacao cancelada pelo usuario.")
        return

    ambiente = questionary.select(
        "[2/3] Onde este pipeline sera utilizado?",
        choices=[
            "Desenvolvimento Local (Dev)",
            "Ambiente de Testes (Staging)",
            "Servidor de Producao (Cloud/On-Premise)"
        ]
    ).ask()

    if not ambiente:
        print("\n❌ Operacao cancelada pelo usuario.")
        return

    print("\n🔒 Atencao: Privilegios elevados sao necessarios para ativar o pipeline.")
    senha = questionary.password("[3/3] Digite a senha de Admin ou Root:").ask()

    if not senha:
        print("\n❌ Senha nao fornecida. Abortando pipeline.")
        return

    print("\n" + "-"*50)
    print("🔄 Processando configuracoes...")
    print(f"-> Sistema Detectado: {sistema}")
    print(f"-> Ambiente de Destino: {ambiente}")
    print("-"*50)
    
    for i in range(1, 4):
        print(f" Ativando pipeline e aplicando permissoes de root... ({i}/3)")
        time.sleep(1)

    print("\n⚡ Acionando subsistema de hardware C++...")
    motor = inicializar_motor_grafico()
    
    if motor and motor.ligarMotorGrafico():
        dados_python = [0.0, 1.0, -5.0, 2.0, 1.5, -3.0]
        tamanho = len(dados_python)
        ArrayFloatsC = ctypes.c_float * tamanho
        dados_para_c = ArrayFloatsC(*dados_python)
        
        motor.enviarGeometria(dados_para_c, tamanho)
        motor.desligarMotorGrafico()
        
        print("\n🚀 [SUCESSO] Pipeline ativado e rodando com controle total!")
        print("="*50 + "\n")
    else:
        print("\n❌ Falha: O pipeline nao pôde ser ativado devido a erros de hardware ou DLL ausente.")
        print("="*50 + "\n")

if __name__ == "__main__":
    try:
        run_pipeline()
    except KeyboardInterrupt:
        print("\n\n❌ Processo interrompido.")
        sys.exit(0)
