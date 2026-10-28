# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# CPUandGPUpipeline - App com janela (Release 2.0.0)
# Verifica se o seu PC e compativel e mede a velocidade de envio de dados ao hardware.
#
#   python app.py            -> abre a janela
#   python app.py --texto    -> mesmo teste, sem janela (terminal)
#
# SEGURANCA: este app so le informacoes e faz um teste pequeno (4 MB por vez).
# Nao instala nada, nao altera configuracoes, nao pede senha, nao usa a internet.

import os
import platform
import queue
import sys
import threading

from main import caminho_biblioteca, carregar_motor

VERSAO = "2.0.0"
TIPOS = {1: "GPU", 2: "CPU"}

TEXTO_SEGURANCA = (
    "Este app so le informacoes do seu PC e faz um teste pequeno de envio de dados. "
    "Ele NAO instala nada, NAO altera configuracoes do sistema, NAO pede senha "
    "e NAO usa a internet."
)

# nivel -> (titulo, cor)
NIVEIS = {
    "ok": ("OK - Compativel", "#1b7f3b"),
    "parcial": ("ATENCAO - Funciona em modo CPU", "#a35c00"),
    "nao": ("NAO COMPATIVEL (seu PC nao foi alterado)", "#b00020"),
}


# ----------------------------- LOGICA (sem janela) -----------------------------

def verificar():
    """Liga o motor e descobre o que o PC tem. Nao altera nada no sistema."""
    r = {
        "sistema": f"{platform.system()} {platform.release()}",
        "arquitetura": platform.machine(),
        "python": platform.python_version(),
        "nivel": "nao",
        "dispositivo": "",
        "tipo": "",
        "mensagem": "",
        "motor": None,
    }
    caminho = caminho_biblioteca()
    if not os.path.exists(caminho):
        r["mensagem"] = ("O arquivo do motor nao foi encontrado: "
                         + os.path.basename(caminho)
                         + ". Baixe o pacote completo na aba Releases.")
        return r
    try:
        motor = carregar_motor(caminho)
    except OSError as e:
        r["mensagem"] = f"Nao consegui carregar o motor: {e}"
        return r

    if not motor.ligarMotorGrafico():
        r["mensagem"] = ("Nenhum driver OpenCL foi encontrado. Instale o driver mais recente da sua "
                         "placa de video, no site oficial (veja o DRIVERS.md), e tente de novo.")
        return r

    r["motor"] = motor
    if hasattr(motor, "nomeDispositivo"):
        r["dispositivo"] = (motor.nomeDispositivo() or b"").decode("utf-8", "replace")
        r["tipo"] = TIPOS.get(motor.tipoDispositivoAtual(), "")
    if r["tipo"] == "GPU":
        r["nivel"] = "ok"
        r["mensagem"] = "Sua placa de video e compativel."
    else:
        r["nivel"] = "parcial"
        r["mensagem"] = ("Nenhuma GPU compativel foi encontrada. O motor funciona, "
                         "mas usando a CPU, sem aceleracao de GPU.")
    return r


def testar_velocidade(motor):
    """Mede MB/s de envio de dados ao hardware. NAO e FPS de jogo. -1 se falhar."""
    if motor is None or not hasattr(motor, "benchmarkTransferencia"):
        return -1.0
    return motor.benchmarkTransferencia(1_000_000, 50)  # 4 MB x 50 envios


def montar_relatorio(r, mbs=None):
    linhas = [
        f"CPU and GPU Pipeline {VERSAO}",
        f"Resultado: {NIVEIS[r['nivel']][0]}",
        f"Sistema: {r['sistema']} ({r['arquitetura']})",
        f"Python: {r['python']}",
    ]
    if r["dispositivo"]:
        linhas.append(f"Dispositivo OpenCL: {r['dispositivo']} ({r['tipo']})")
    linhas.append(f"Detalhe: {r['mensagem']}")
    if mbs is not None:
        if mbs > 0:
            linhas.append(f"Velocidade de envio de dados: {mbs:,.0f} MB/s")
            linhas.append("(Isso mede so o envio de dados ao hardware. Nao e FPS de jogo.)")
        else:
            linhas.append("Teste de velocidade: falhou.")
    return "\n".join(linhas)


def desligar(r):
    if r and r.get("motor") is not None:
        try:
            r["motor"].desligarMotorGrafico()
        except Exception:
            pass


# ----------------------------- MODO TEXTO -----------------------------

def modo_texto():
    r = verificar()
    mbs = testar_velocidade(r["motor"]) if r["nivel"] != "nao" else None
    print(montar_relatorio(r, mbs))
    desligar(r)
    return 1 if r["nivel"] == "nao" else 0


# ----------------------------- JANELA -----------------------------

def abrir_janela():
    import tkinter as tk
    from tkinter import ttk

    root = tk.Tk()
    root.title(f"CPU and GPU Pipeline {VERSAO}")
    root.geometry("600x560")
    root.minsize(520, 480)

    estado = {"r": None, "mbs": None, "ocupado": False}

    corpo = ttk.Frame(root, padding=14)
    corpo.pack(fill="both", expand=True)

    ttk.Label(corpo, text="CPU and GPU Pipeline", font=("TkDefaultFont", 18, "bold")).pack(anchor="w")
    ttk.Label(corpo, text="Verificador de compatibilidade e teste de velocidade").pack(anchor="w", pady=(0, 10))

    caixa = ttk.LabelFrame(corpo, text="Seguranca", padding=8)
    caixa.pack(fill="x")
    ttk.Label(caixa, text=TEXTO_SEGURANCA, wraplength=540, justify="left").pack(anchor="w")

    status = tk.Label(corpo, text="Verificando seu PC...", font=("TkDefaultFont", 13, "bold"),
                      fg="white", bg="#555555", pady=10)
    status.pack(fill="x", pady=10)

    detalhes = tk.Text(corpo, height=10, wrap="word", state="disabled")
    detalhes.pack(fill="both", expand=True)

    barra = ttk.Progressbar(corpo, mode="indeterminate")

    botoes = ttk.Frame(corpo)
    botoes.pack(fill="x", pady=(10, 0))

    def escrever(texto):
        detalhes.configure(state="normal")
        detalhes.delete("1.0", "end")
        detalhes.insert("1.0", texto)
        detalhes.configure(state="disabled")

    def definir_botoes(modo):
        for b in lista_botoes:
            b.configure(state=modo)

    def trabalhar(func, ao_terminar):
        """Roda func em outra thread (a janela nao trava) e chama ao_terminar no fim."""
        if estado["ocupado"]:
            return
        estado["ocupado"] = True
        definir_botoes("disabled")
        barra.pack(fill="x", pady=(8, 0))
        barra.start(12)
        q = queue.Queue()

        def alvo():
            try:
                q.put(("ok", func()))
            except Exception as e:  # mostra o erro em vez de fechar a janela
                q.put(("erro", e))

        def checar():
            try:
                tipo, valor = q.get_nowait()
            except queue.Empty:
                root.after(100, checar)
                return
            barra.stop()
            barra.pack_forget()
            estado["ocupado"] = False
            definir_botoes("normal")
            if tipo == "erro":
                status.configure(text="ERRO inesperado", bg=NIVEIS["nao"][1])
                escrever(f"Aconteceu um erro: {valor}\nSeu PC nao foi alterado.")
            else:
                ao_terminar(valor)

        threading.Thread(target=alvo, daemon=True).start()
        root.after(100, checar)

    def mostrar(r, mbs=None):
        titulo, cor = NIVEIS[r["nivel"]]
        status.configure(text=titulo, bg=cor)
        escrever(montar_relatorio(r, mbs))

    def ao_verificar(r):
        desligar(estado["r"])
        estado["r"], estado["mbs"] = r, None
        mostrar(r)

    def ao_testar(mbs):
        estado["mbs"] = mbs
        mostrar(estado["r"], mbs)

    def acao_verificar():
        trabalhar(verificar, ao_verificar)

    def acao_testar():
        r = estado["r"]
        if not r:
            escrever("Primeiro clique em 'Verificar meu PC'.")
            return
        if r["nivel"] == "nao":  # mantem o diagnostico na tela
            escrever(montar_relatorio(r) + "\n\nO teste de velocidade so roda em PCs compativeis.")
            return
        trabalhar(lambda: testar_velocidade(r["motor"]), ao_testar)

    def acao_copiar():
        if not estado["r"]:
            return
        root.clipboard_clear()
        root.clipboard_append(montar_relatorio(estado["r"], estado["mbs"]))
        status.configure(text="Relatorio copiado!", bg="#1b4f8a")

    def sair():
        desligar(estado["r"])
        root.destroy()

    b1 = ttk.Button(botoes, text="Verificar meu PC", command=acao_verificar)
    b2 = ttk.Button(botoes, text="Testar velocidade", command=acao_testar)
    b3 = ttk.Button(botoes, text="Copiar relatorio", command=acao_copiar)
    b4 = ttk.Button(botoes, text="Sair", command=sair)
    lista_botoes = [b1, b2, b3, b4]
    for b in lista_botoes:
        b.pack(side="left", padx=(0, 8))

    root.protocol("WM_DELETE_WINDOW", sair)
    root.after(200, acao_verificar)  # ja verifica ao abrir
    root.mainloop()


def main(argv):
    if "--texto" in argv:
        return modo_texto()
    try:
        import tkinter
    except ImportError:
        print("A janela precisa do tkinter (Linux: sudo apt install python3-tk). Usando o modo texto.\n")
        return modo_texto()
    try:
        abrir_janela()
    except tkinter.TclError as e:
        print(f"Nao consegui abrir a janela ({e}). Usando o modo texto.\n")
        return modo_texto()
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except KeyboardInterrupt:
        sys.exit(130)
