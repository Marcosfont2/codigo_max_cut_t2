#!/bin/bash
# Gera as instancias em instancias_t2/ (deterministico: sementes fixas).
# Uso (a partir da raiz do projeto): bash scripts/gerar_instancias.sh
set -e
g++ -std=c++17 -O2 -o gerador_t2 gerador_t2.cpp
mkdir -p instancias_t2
# Familia ALE: grafos aleatorios esparsos (m = 2n), pesos +1, 3 sementes por tamanho
# Familia PM1: idem, pesos +1/-1 com probabilidade 1/2
for n in 20 30 40 50 60 70 80 90 100 110 120; do
  for s in 1 2 3; do
    nn=$(printf "%03d" $n)
    ./gerador_t2 aleatorio $n $((2*n)) $s 1   > instancias_t2/ALE_n${nn}_s$s
    ./gerador_t2 aleatorio $n $((2*n)) $s pm1 > instancias_t2/PM1_n${nn}_s$s
  done
done
# Familia TORO: grades toroidais 2D (linhas x colunas) com pesos +1/-1 (semente 1)
for dim in "5 20" "5 100" "5 200" "5 400" "6 34" "6 84" "6 167" "6 334" "8 25" "8 63" "8 125" "8 250" "10 200"; do
  set -- $dim
  ./gerador_t2 toro $1 $2 1 > instancias_t2/TORO_$1x$2
done
echo "$(ls instancias_t2 | wc -l) instancias geradas em instancias_t2/"
