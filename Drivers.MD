# Drivers e ferramentas

O projeto usa **OpenCL**. Ele funciona se o PC tiver um driver OpenCL instalado.
Sem driver, o programa avisa "Nenhum driver OpenCL instalado" e nao trava.

> Baixe drivers **somente nos sites oficiais** .
> Nunca baixe "driver" de link de comentario, anuncio ou site desconhecido.

## Drivers de video (ja trazem OpenCL)
| Placa | Onde baixar |
|-------|-------------|
| NVIDIA | https://www.nvidia.com/drivers |
| AMD | https://www.amd.com/en/support |
| Intel | https://www.intel.com/content/www/us/en/download-center/home.html |

No Windows, instale o driver mais recente da sua placa. O OpenCL vem junto.

## Linux (Ubuntu/Debian)
```
sudo apt update
sudo apt install g++ make ocl-icd-opencl-dev opencl-headers clinfo
```
Driver OpenCL (escolha o do seu hardware):
- NVIDIA: driver proprietario da NVIDIA
- Intel: `sudo apt install intel-opencl-icd`
- Sem GPU compativel (modo CPU): `sudo apt install pocl-opencl-icd`

Para conferir se o PC enxerga o OpenCL, rode `clinfo`.

## Windows: compilar
1. Instale o MSYS2: https://www.msys2.org
2. No terminal "MSYS2 MINGW64":
```
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-opencl-headers mingw-w64-x86_64-opencl-icd make
```
3. Coloque `C:\msys64\mingw64\bin` no PATH e rode `build.bat` (ou `build.bat exe`).

> O modo "CPU de fallback" no Windows so funciona se houver um runtime OpenCL de CPU instalado.
> Sem GPU e sem runtime, o programa avisa que nao ha driver.

## macOS
A Apple abandonou o OpenCL (descontinuado). O Makefile ja tem o caminho do macOS,
mas ele **nao foi testado** nesta versao.
