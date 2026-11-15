# Corte Máximo — algoritmos exatos (Trabalho 2, DIM0605)

Dois algoritmos exatos para o Max-Cut, comparados com a heurística gulosa + busca local do Trabalho 1:

1. **Branch-and-Bound** com limitante superior incremental, inicializado com a heurística do Trabalho 1.
2. **Programação dinâmica sobre a fronteira** de uma ordem dos vértices (largura de caminho).

Não é preciso instalar nada além de um compilador C++17 (`g++`). O relatório em PDF está em `relatorio/relatorio.pdf`.

## Arquivos

- `exato.cpp`: programa principal (os dois algoritmos, a heurística do Trabalho 1 e a interface).
- `gerador_t2.cpp`: gerador de instâncias (aleatórias e grades toroidais). `gerador.cpp` e `maxcut.cpp` são os do Trabalho 1.
- `validacao.cpp`: confere os algoritmos contra força bruta em grafos pequenos aleatórios.
- `instancias_t2/`: 79 instâncias do Trabalho 2. `instancias_t1/`: instâncias do Trabalho 1 (G-set e R100k).
- `resultados_t2/`: relatório de cada execução (valor, partição) e `resultados.csv`.
- `scripts/`: gerar instâncias, rodar todos os experimentos, gerar tabelas e figura do relatório.
- `relatorio/`: fontes LaTeX e PDF.

Formato das instâncias (igual ao Trabalho 1): primeira linha `n m`, depois `m` linhas `u v peso`, vértices de 1 a n.

## Compilar

```
g++ -std=c++17 -O2 -o exato exato.cpp
g++ -std=c++17 -O2 -o gerador_t2 gerador_t2.cpp
g++ -std=c++17 -O2 -o validacao validacao.cpp
```

## Rodar

```
./exato                                  # menu: escolhe a instância dentre instancias_t2/ e instancias_t1/
./exato instancias_t2/TORO_8x250         # uma instância; relatório em tela e em resultados_t2/TORO_8x250.txt
./exato <instancia> [saida] [opções]
```

Opções:
- `--algoritmo bb|dp|ambos` (padrão `ambos`)
- `--tempo S` limite de tempo por algoritmo, em segundos (padrão 60)
- `--largura-max W` largura máxima aceita pela DP (padrão 26; a tabela tem 2^W entradas)
- `--memoria-max MB` (padrão 2000). Em máquinas com pouca memória, use `--largura-max 22`.
- `--csv arquivo` acrescenta uma linha de resultados ao CSV
- `--so-largura` só estima a largura da ordem (diz se a DP é viável)

A saída mostra o valor de cada algoritmo, se a otimalidade foi provada, o valor ótimo, a razão heurística/ótimo, a conferência do valor recalculado a partir da partição, e a partição (um caractere 0/1 por vértice, 1..n). O arquivo de saída traz a partição no formato `vértice lado`.

Os tempos do relatório foram medidos em uma máquina equipada com processador Intel Core i5-9300H @ 2,40 GHz e 8 GB de RAM (em execução single-thread)
