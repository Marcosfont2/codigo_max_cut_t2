#!/usr/bin/env python3
"""Figura de escala: tempo do B&B e da DP em funcao de n (grafos aleatorios esparsos). Uso: python3 scripts/gerar_figuras.py"""
import csv, statistics as st
from collections import defaultdict
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
rows = list(csv.DictReader(open("resultados_longos/resultados.csv")))
AZUL, CINZA = "#1f5fa8", "#555555"
fig, axs = plt.subplots(1, 2, figsize=(7.4, 2.9), sharey=True)
for ax, fam, tit in zip(axs, ("ALE", "PM1"), ("pesos $+1$", "pesos $\\pm1$")):
    bb, dp = defaultdict(list), defaultdict(list)
    for r in rows:
        if r["instancia"].startswith(fam + "_"):
            n = int(r["n"])
            bb[n].append(float(r["bb_tempo"])); dp[n].append(float(r["dp_tempo"]))
    ns = sorted(bb)
    ax.plot(ns, [st.mean(bb[n]) for n in ns], "o-", color=AZUL, ms=3.5, label="Branch-and-Bound (limite 30 s)")
    ax.plot(ns, [max(st.mean(dp[n]), 1e-4) for n in ns], "s-", color=CINZA, ms=3.5, label="DP sobre a fronteira")
    ax.axhline(30, color=AZUL, lw=0.6, ls=":")
    ax.set_yscale("log"); ax.set_xlabel("$n$"); ax.set_title(f"Aleatórias esparsas, {tit}", fontsize=9)
    ax.grid(alpha=0.25)
axs[0].set_ylabel("tempo médio (s)")
axs[0].legend(fontsize=7, loc="upper left")
plt.tight_layout()
plt.savefig("relatorio/fig_escala.pdf")
print("relatorio/fig_escala.pdf")
