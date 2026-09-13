@echo off
echo Compilando o motor grafico para DLL...
g++ -O3 -shared -o motor_grafico.dll processamento.cpp -lOpenCL
if %errorlevel% equ 0 (
    echo ✅ DLL gerada com sucesso!
) else (
    echo ❌ Erro na compilacao. Verifique se o g++ e o SDK do OpenCL estao instalados.
)
pause
