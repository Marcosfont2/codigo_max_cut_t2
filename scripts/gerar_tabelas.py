#!/usr/bin/env python3
"""Le resultados_t2/resultados.csv (e largura_T1.txt) e escreve relatorio/tabelas.tex com as tabelas do relatorio.
Uso (raiz do projeto): python3 scripts/gerar_tabelas.py"""
import csv, re, statistics as st
from collections import defaultdict

def num(x, casas=3):
    return f"{x:.{casas}f}".replace(".", "{,}")

def tempo(x):
    if x < 0.0005: return "$<$0{,}001"
    if x < 10: return num(x, 3)
    if x < 100: return num(x, 1)
    return f"{x:.0f}"

def inteiro(x):
    return f"{int(round(x)):,}".replace(",", "\\,")

def nos(x):
    if x >= 1e6: return f"{x/1e6:.1f}".replace(".", "{,}") + "\\,M"
    return inteiro(x)

linhas = list(csv.DictReader(open("resultados_t2/resultados.csv")))
for r in linhas:
    for k in ("n", "m", "peso_total", "heur"):
        r[k] = int(r[k])
    for k in ("t_heur", "bb_tempo", "dp_tempo"):
        r[k] = float(r[k]) if r[k] != "" else None
    r["bb_valor"] = int(r["bb_valor"]) if r["bb_valor"] != "" else None
    r["bb_otimo"] = r["bb_otimo"] == "1"
    r["bb_nos"] = int(r["bb_nos"]) if r["bb_nos"] != "" else None
    r["dp_largura"] = int(r["dp_largura"]) if r["dp_largura"] != "" else None
    r["dp_ok"] = r["dp_ok"] == "1"
    r["dp_valor"] = int(r["dp_valor"]) if r["dp_ok"] else None
    r["otimo"] = int(r["otimo"]) if r["otimo"] != "" else None

out = []
# ---------- Tabela: familias aleatorias, agregadas por n (3 sementes) ----------
for fam, titulo, rotulo in (("ALE", "Grafos aleat\\'orios esparsos ($m=2n$), pesos $+1$", "tab:ale"),
                            ("PM1", "Grafos aleat\\'orios esparsos ($m=2n$), pesos $\\pm1$", "tab:pm1")):
    grupos = defaultdict(list)
    for r in linhas:
        if r["instancia"].startswith(fam + "_"):
            grupos[r["n"]].append(r)
    out.append("\\begin{table}[ht]\\centering\\small")
    out.append(f"\\caption{{{titulo}. M\\'edias sobre 3 sementes por tamanho. Raz\\~ao: G+BL/\\'otimo (heur\\'istica do Trabalho 1). "
               "B\\&B: inst\\^ancias resolvidas dentre 3 (limite de 30 s) e tempo m\\'edio das resolvidas; "
               "DP: largura $w$ m\\'edia e tempo m\\'edio.}\\label{" + rotulo + "}")
    out.append("\\begin{tabular}{rcccccc}\\hline")
    out.append("$n$ & \\'Otimo (m\\'ed.) & Raz\\~ao & B\\&B resolv. & B\\&B tempo (s) & B\\&B n\\'os & DP $w$ / tempo (s) \\\\\\hline")
    for n in sorted(grupos):
        g = grupos[n]
        com_otimo = [r for r in g if r["otimo"] is not None]
        otm = st.mean(r["otimo"] for r in com_otimo) if com_otimo else None
        raz = st.mean(r["heur"] / r["otimo"] for r in com_otimo if r["otimo"]) if com_otimo else None
        res = [r for r in g if r["bb_otimo"]]
        bbt = tempo(st.mean(r["bb_tempo"] for r in res)) if res else "---"
        bbn = nos(st.mean(r["bb_nos"] for r in res)) if res else "---"
        dps = [r for r in g if r["dp_ok"]]
        if len(dps) == len(g):
            dpc = f"{st.mean(r['dp_largura'] for r in g):.1f}".replace(".", "{,}") + " / " + tempo(st.mean(r["dp_tempo"] for r in g))
        elif dps:
            dpc = f"{len(dps)}/3 resolv."
        else:
            dpc = "invi\\'avel"
        out.append(f"{n} & {num(otm,1) if otm is not None else '---'} & {num(raz) if raz else '---'} & {len(res)}/3 & {bbt} & {bbn} & {dpc} \\\\")
    out.append("\\hline\\end{tabular}\\end{table}\n")

# ---------- Tabela: toroidais ----------
tor = [r for r in linhas if r["instancia"].startswith("TORO_")]
tor.sort(key=lambda r: (r["n"], r["instancia"]))
out.append("\\begin{table}[ht]\\centering\\small")
out.append("\\caption{Grades toroidais 2D com pesos $\\pm1$ (inst\\^ancias \\texttt{TORO\\_linhas x colunas}). "
           "G+BL: heur\\'istica do Trabalho 1; B\\&B: limite de 30 s (``---'' = n\\~ao provou a otimalidade; entre par\\^enteses, o melhor valor encontrado).}"
           "\\label{tab:toro}")
out.append("\\begin{tabular}{lrrrcrcrr}\\hline")
out.append("Inst. & $n$ & \\'Otimo & G+BL & Raz\\~ao & $w$ & DP (s) & B\\&B (s) & n\\'os B\\&B \\\\\\hline")
for r in tor:
    nome = r["instancia"].replace("TORO_", "").replace("x", "$\\times$")
    otm = r["otimo"]
    bbc = tempo(r["bb_tempo"]) if r["bb_otimo"] else f"--- ({r['bb_valor']})"
    out.append(f"{nome} & {r['n']} & {otm if otm is not None else '---'} & {r['heur']} & "
               f"{num(r['heur']/otm) if otm else '---'} & {r['dp_largura'] if r['dp_largura'] else '---'} & "
               f"{tempo(r['dp_tempo']) if r['dp_ok'] else '---'} & {bbc} & {nos(r['bb_nos'])} \\\\")
out.append("\\hline\\end{tabular}\\end{table}\n")

# ---------- Tabela: largura estimada das instancias do Trabalho 1 ----------
out_t1 = []
try:
    ref = {"G32": 1410, "G35": 7687, "G55": 10299, "G57": 3494, "G60": 14188, "G62": 4870, "G65": 5562,
           "G67": 6940, "G70": 9595, "G72": 6998, "G77": 9926, "G81": 14060}
    out_t1.append("\\begin{table}[ht]\\centering\\small")
    out_t1.append("\\caption{Largura de fronteira encontrada para as inst\\^ancias do Trabalho 1. ``$>60$'' indica que a busca desistiu ao passar de 60 "
               "(uma tabela com $2^{60}$ entradas \\'e invi\\'avel; o limite pr\\'atico da implementa\\c{c}\\~ao \\'e $w\\le 26$).}\\label{tab:largT1}")
    out_t1.append("\\begin{tabular}{lrrc}\\hline Inst. & $n$ & $m$ & Largura $w$ \\\\\\hline")
    for linha in open("resultados_t2/largura_T1.txt"):
        mm = re.match(r"(\S+) n=(\d+) m=(\d+) largura_estimada=(\S+)", linha)
        if mm:
            nome, n, m, w = mm.groups()
            w = w.replace(">", "$>$")
            out_t1.append(f"{nome} & {n} & {m} & {w} \\\\")
    out_t1.append("\\hline\\end{tabular}\\end{table}\n")
except FileNotFoundError:
    pass

open("relatorio/tabelas.tex", "w").write("\n".join(out))
open("relatorio/tab_largT1.tex", "w").write("\n".join(out_t1))
print("relatorio/tabelas.tex gerado")
