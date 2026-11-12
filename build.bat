@echo off
setlocal
set /p VERSION=<VERSION
echo Building CPUandGPUpipeline v%VERSION%...
set OPENCL_FLAGS=
set OPENCL_LIBS=
if defined OPENCL_ROOT (
  set OPENCL_FLAGS=-DPIPELINE_WITH_OPENCL -I"%OPENCL_ROOT%\include"
  set OPENCL_LIBS=-L"%OPENCL_ROOT%\lib" -lOpenCL
)
g++ -O2 -std=c++17 -Wall -Wextra -DPIPELINE_BUILD -DAPP_VERSION=\"%VERSION%\" %OPENCL_FLAGS% -shared -static-libgcc -static-libstdc++ -o motor_grafico.dll processamento.cpp %OPENCL_LIBS% -pthread
if errorlevel 1 (
  echo [ERROR] Build failed. Check MinGW and, if using OpenCL, OPENCL_ROOT.
  exit /b 1
)
echo [OK] motor_grafico.dll created.
endlocal
