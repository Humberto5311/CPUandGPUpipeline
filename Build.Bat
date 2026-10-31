@echo off
set /p VERSION=<VERSION
echo Compilando o motor grafico v%VERSION% para DLL...
g++ -O3 -std=c++17 -Wall -Wextra -DAPP_VERSION=\"%VERSION%\" -shared -static-libgcc -static-libstdc++ -o motor_grafico.dll processamento.cpp -lOpenCL
if %errorlevel% equ 0 (
    echo [OK] DLL gerada com sucesso: motor_grafico.dll
) else (
    echo [ERRO] Erro na compilacao. Verifique se o g++ e o SDK do OpenCL estao instalados.
)
pause
