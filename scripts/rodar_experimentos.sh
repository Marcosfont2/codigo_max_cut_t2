#!/bin/bash
# Roda os dois algoritmos exatos (e a heuristica do Trabalho 1) em todas as instancias de instancias_t2/
# e grava resultados_t2/resultados.csv. Tambem estima a largura das instancias do Trabalho 1.
# O script e RETOMAVEL: instancias que ja estao no CSV sao puladas. Para recomecar do zero,
# apague resultados_t2/resultados.csv antes de rodar.
# Uso (raiz do projeto): bash scripts/rodar_experimentos.sh [tempo_limite_s]   (padrao 30)
set -e
T=${1:-30}
g++ -std=c++17 -O2 -o exato exato.cpp
mkdir -p resultados_t2
touch resultados_t2/resultados.csv
for f in instancias_t2/*; do
  nome=$(basename "$f")
  if grep -q "^$nome," resultados_t2/resultados.csv; then continue; fi
  ./exato "$f" "resultados_t2/$nome.txt" --tempo $T --largura-max 26 --csv resultados_t2/resultados.csv > /dev/null
  echo "ok $nome"
done
# Largura estimada das instancias do Trabalho 1 (G-set e R100k): a DP so e viavel com largura pequena
if [ ! -s resultados_t2/largura_T1.txt ]; then
  for f in instancias_t1/*; do
    ./exato "$f" --so-largura --largura-max 60 >> resultados_t2/largura_T1.txt
  done
fi
echo "Concluido."
