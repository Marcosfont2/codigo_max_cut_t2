
//  Branch-and-Bound com limitante superior incremental, inicializado com a heuristica do Trabalho 1.
//  Programacao dinamica sobre a "fronteira" de uma ordem de processamento dos vertices

#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
using ll = long long;
using Clock = chrono::steady_clock;

//  Grafo e utilitarios

struct Grafo {
    int n = 0;
    ll m = 0;          // numero de arestas (apos fundir arestas paralelas)
    ll pesoTotal = 0;  // soma dos pesos (com sinal)
    ll pesoAbs = 0;    // soma dos |pesos|
    vector<vector<pair<int, int>>> adj;  // (vizinho, peso)
};

static double segundos(Clock::time_point a, Clock::time_point b) { return chrono::duration<double>(b - a).count(); }

// Formato: "n m" e depois m linhas "u v peso" (vertices de 1 a n). Ignora lacos e funde arestas paralelas.
bool lerGrafo(const string &caminho, Grafo &g) {
    ifstream in(caminho);
    if (!in) return false;
    ll m;
    if (!(in >> g.n >> m) || g.n <= 0) return false;
    g.adj.assign(g.n, {});
    for (ll i = 0; i < m; i++) {
        int u, v, w;
        if (!(in >> u >> v >> w)) return false;
        u--, v--;
        if (u < 0 || v < 0 || u >= g.n || v >= g.n) return false;
        if (u == v) continue;
        g.adj[u].push_back({v, w});
        g.adj[v].push_back({u, w});
    }
    g.m = 0;
    for (auto &lista : g.adj) {  // funde paralelas: ordena por vizinho e soma os pesos
        sort(lista.begin(), lista.end());
        vector<pair<int, int>> fund;
        for (auto &[u, w] : lista) {
            if (!fund.empty() && fund.back().first == u) fund.back().second += w;
            else fund.push_back({u, w});
        }
        lista = fund;
        g.m += lista.size();
    }
    g.m /= 2;
    for (int v = 0; v < g.n; v++)
        for (auto [u, w] : g.adj[v])
            if (u > v) {
                g.pesoTotal += w;
                g.pesoAbs += abs(w);
            }
    return true;
}

// Valor do corte: soma dos pesos das arestas cujas pontas estao em lados diferentes.
ll valorCorte(const Grafo &g, const vector<int> &lado) {
    ll corte = 0;
    for (int v = 0; v < g.n; v++)
        for (auto [u, w] : g.adj[v])
            if (u > v && lado[u] != lado[v]) corte += w;
    return corte;
}


//  Heuristica do Trabalho 1 (gulosa por grau + busca local 1-flip), usada como limite inferior
//  inicial do Branch-and-Bound e como referencia de comparacao.

vector<int> ordemPorGrau(const Grafo &g) {
    vector<int> ordem(g.n);
    iota(ordem.begin(), ordem.end(), 0);
    sort(ordem.begin(), ordem.end(), [&](int a, int b) { return g.adj[a].size() > g.adj[b].size(); });
    return ordem;
}

vector<int> construcaoGulosa(const Grafo &g, const vector<int> &ordem) {
    vector<int> lado(g.n, -1);
    for (int v : ordem) {
        ll peso0 = 0, peso1 = 0;
        for (auto [u, w] : g.adj[v]) {
            if (lado[u] == 0) peso0 += w;
            else if (lado[u] == 1) peso1 += w;
        }
        lado[v] = (peso0 >= peso1) ? 1 : 0;
    }
    return lado;
}

ll ganhoDeTroca(const Grafo &g, const vector<int> &lado, int v) {
    ll ganho = 0;
    for (auto [u, w] : g.adj[v]) ganho += (lado[u] == lado[v]) ? w : -w;
    return ganho;
}

void buscaLocal(const Grafo &g, vector<int> &lado, const vector<int> &ordem) {
    deque<int> fila(ordem.begin(), ordem.end());
    vector<char> naFila(g.n, 1);
    while (!fila.empty()) {
        int v = fila.front();
        fila.pop_front();
        naFila[v] = 0;
        if (ganhoDeTroca(g, lado, v) <= 0) continue;
        lado[v] ^= 1;
        for (auto [u, w] : g.adj[v])
            if (!naFila[u]) {
                naFila[u] = 1;
                fila.push_back(u);
            }
    }
}

vector<int> heuristicaT1(const Grafo &g) {
    vector<int> ordem = ordemPorGrau(g);
    vector<int> lado = construcaoGulosa(g, ordem);
    buscaLocal(g, lado, ordem);
    return lado;
}

//  Algoritmo 1: Branch-and-Bound

struct ResultadoBB {
    ll valor = 0;
    vector<int> lado;
    bool otimo = false;  // true se a busca terminou (otimalidade provada)
    ll nos = 0;
    double tempo = 0;
};

// Ordem de decisao dos vertices: o proximo e o que tem maior peso (|w|) para vertices ja ordenados,
// de modo que as arestas fiquem "decididas" cedo e o limitante se torne justo rapidamente.
// novaComp[i] indica que o i-esimo vertice abre uma nova componente conexa (simetria: pode fixar lado 0).
static vector<int> ordemDecisao(const Grafo &g, vector<char> &novaComp) {
    int n = g.n;
    vector<ll> ligado(n, 0), grauPond(n, 0);
    for (int v = 0; v < n; v++)
        for (auto [u, w] : g.adj[v]) grauPond[v] += abs(w);
    vector<char> usado(n, 0);
    vector<int> ordem;
    novaComp.assign(n, 0);
    for (int i = 0; i < n; i++) {
        int melhor = -1;
        for (int v = 0; v < n; v++) {
            if (usado[v]) continue;
            if (melhor < 0 || ligado[v] > ligado[melhor] || (ligado[v] == ligado[melhor] && grauPond[v] > grauPond[melhor]))
                melhor = v;
        }
        novaComp[i] = (ligado[melhor] == 0);
        usado[melhor] = 1;
        ordem.push_back(melhor);
        for (auto [u, w] : g.adj[melhor]) ligado[u] += abs(w);
    }
    return ordem;
}

struct BranchAndBound {
    const Grafo &g;
    int n;
    vector<int> ordem;
    vector<char> novaComp;
    vector<int> lado;           // -1 = nao decidido, 0 ou 1
    vector<array<ll, 2>> c;     // c[v][s] = peso das arestas de v para vizinhos ja decididos no lado s
    ll corte = 0;               // valor das arestas ja decididas (ambas as pontas decididas) que estao cortadas
    ll somaMax = 0;             // soma, sobre v nao decidido, de max(c[v][0], c[v][1])
    ll ubNN = 0;                // soma de max(w, 0) das arestas com as duas pontas nao decididas
    ll melhor = 0;
    vector<int> melhorLado;
    ll nos = 0;
    bool estourou = false;
    double limite;
    Clock::time_point t0;

    BranchAndBound(const Grafo &grafo, double limiteTempo) : g(grafo), n(grafo.n), limite(limiteTempo) {
        ordem = ordemDecisao(g, novaComp);
        lado.assign(n, -1);
        c.assign(n, {0, 0});
        for (int v = 0; v < n; v++)
            for (auto [u, w] : g.adj[v])
                if (u > v) ubNN += max(w, 0);
    }

    // Decide v no lado s, atualizando de forma incremental o corte e as parcelas do limitante. O(grau(v)).
    void colocar(int v, int s) {
        corte += c[v][1 - s];  // arestas de v para decididos do lado oposto passam a ser cortadas
        somaMax -= max(c[v][0], c[v][1]);
        lado[v] = s;
        for (auto [u, w] : g.adj[v]) {
            if (lado[u] >= 0) continue;
            ubNN -= max(w, 0);
            somaMax -= max(c[u][0], c[u][1]);
            c[u][s] += w;
            somaMax += max(c[u][0], c[u][1]);
        }
    }
    void desfazer(int v, int s) {
        lado[v] = -1;
        for (auto [u, w] : g.adj[v]) {
            if (lado[u] >= 0) continue;
            somaMax -= max(c[u][0], c[u][1]);
            c[u][s] -= w;
            somaMax += max(c[u][0], c[u][1]);
            ubNN += max(w, 0);
        }
        somaMax += max(c[v][0], c[v][1]);
        corte -= c[v][1 - s];
    }

    void rec(int i) {
        if (estourou) return;
        if ((++nos & 4095) == 0 && segundos(t0, Clock::now()) > limite) {
            estourou = true;
            return;
        }
        // Limitante superior: nenhum completamento da solucao parcial passa deste valor.
        if (corte + somaMax + ubNN <= melhor) return;
        if (i == n) {  // aqui somaMax = ubNN = 0, logo corte > melhor
            melhor = corte;
            melhorLado = lado;
            return;
        }
        int v = ordem[i];
        int primeiro = (c[v][1] >= c[v][0]) ? 0 : 1;  // lado que corta mais peso primeiro
        for (int k = 0; k < 2; k++) {
            int s = k == 0 ? primeiro : 1 - primeiro;
            if (novaComp[i] && s == 1) continue;  // quebra de simetria por componente
            colocar(v, s);
            rec(i + 1);
            desfazer(v, s);
            if (estourou) return;
        }
    }
};

ResultadoBB branchAndBound(const Grafo &g, const vector<int> &ladoInicial, double limiteTempo) {
    BranchAndBound bb(g, limiteTempo);
    bb.t0 = Clock::now();
    bb.melhorLado = ladoInicial;
    bb.melhor = valorCorte(g, ladoInicial);
    bb.rec(0);
    ResultadoBB r;
    r.valor = bb.melhor;
    r.lado = bb.melhorLado;
    r.otimo = !bb.estourou;
    r.nos = bb.nos;
    r.tempo = segundos(bb.t0, Clock::now());
    return r;
}

//  Algoritmo 2: Programacao dinamica sobre a fronteira (largura de caminho)

// Ordem de processamento escolhida de forma gulosa para minimizar a fronteira F (vertices ja processados
// que ainda tem vizinho nao processado). A tabela da DP tem 2^K entradas, K = |F| + 1 no pior passo.

static int ordemFronteiraGulosa(const Grafo &g, int inicio, mt19937 &rng, int corte, vector<int> &ordem) {
    int n = g.n;
    vector<int> cnt(n);  // vizinhos ainda nao processados
    for (int v = 0; v < n; v++) cnt[v] = g.adj[v].size();
    vector<char> proc(n, 0), noF(n, 0), ehCand(n, 0);
    vector<int> cand;
    ordem.clear();
    int tamF = 0, largura = 0, proxLivre = 0;
    for (int t = 0; t < n; t++) {
        int v = -1;
        if (t == 0) {
            v = inicio;
        } else {
            size_t k = 0;
            for (int x : cand)
                if (!proc[x]) cand[k++] = x;
            cand.resize(k);
            if (cand.empty()) {  // nova componente conexa
                while (proc[proxLivre]) proxLivre++;
                v = proxLivre;
            } else {
                int melhorDelta = INT_MAX, melhorCnt = INT_MAX;
                uint32_t melhorSorteio = 0;
                for (int x : cand) {
                    int d = cnt[x] > 0 ? 1 : 0;
                    for (auto [u, w] : g.adj[x])
                        if (noF[u] && cnt[u] == 1) d--;
                    uint32_t sorteio = rng();
                    if (d < melhorDelta || (d == melhorDelta && (cnt[x] < melhorCnt || (cnt[x] == melhorCnt && sorteio < melhorSorteio)))) {
                        melhorDelta = d, melhorCnt = cnt[x], melhorSorteio = sorteio, v = x;
                    }
                }
            }
        }
        largura = max(largura, tamF + 1);
        if (largura > corte) return INT_MAX;
        proc[v] = 1;
        ordem.push_back(v);
        for (auto [u, w] : g.adj[v]) cnt[u]--;
        if (cnt[v] > 0) noF[v] = 1, tamF++;
        for (auto [u, w] : g.adj[v]) {
            if (noF[u] && cnt[u] == 0) noF[u] = 0, tamF--;
            if (!proc[u] && !ehCand[u]) ehCand[u] = 1, cand.push_back(u);
        }
    }
    return largura;
}

struct OrdemFronteira {
    vector<int> ordem;
    int largura = INT_MAX;  // INT_MAX = nenhuma ordem com largura <= corte foi encontrada
    double tempo = 0;
};

// Varias partidas (vertice inicial e desempates diferentes); fica com a de menor largura.
OrdemFronteira melhorOrdemFronteira(const Grafo &g, int tentativas, int corte, double tempoMax) {
    auto t0 = Clock::now();
    mt19937 rng(12345);
    OrdemFronteira melhor;
    vector<int> ordem;
    int inicioGrauMin = 0;
    for (int v = 0; v < g.n; v++)
        if (g.adj[v].size() < g.adj[inicioGrauMin].size()) inicioGrauMin = v;
    for (int i = 0; i < tentativas; i++) {
        int inicio = (i == 0) ? inicioGrauMin : (int)(rng() % g.n);
        int limite = min(corte, melhor.largura == INT_MAX ? corte : melhor.largura - 1);
        int w = ordemFronteiraGulosa(g, inicio, rng, limite, ordem);
        if (w != INT_MAX && w < melhor.largura) melhor.largura = w, melhor.ordem = ordem;
        if (segundos(t0, Clock::now()) > tempoMax) break;
    }
    melhor.tempo = segundos(t0, Clock::now());
    return melhor;
}

struct ResultadoDP {
    bool ok = false;
    string motivo;  // se !ok
    ll valor = 0;
    vector<int> lado;
    int largura = 0;
    double tempo = 0, memMB = 0;
};

ResultadoDP dpLargura(const Grafo &g, const vector<int> &ordem, int larguraMax, double limiteTempo, double memMaxMB) {
    ResultadoDP r;
    int n = g.n;
    if (g.pesoAbs > 1000000000LL) {
        r.motivo = "soma dos pesos grande demais para a tabela de 32 bits";
        return r;
    }
    // --- simulacao previa: largura e memoria necessaria para reconstruir a solucao ---
    {
        vector<int> cnt(n);
        for (int v = 0; v < n; v++) cnt[v] = g.adj[v].size();
        vector<char> proc(n, 0), noF(n, 0);
        int tamF = 0, K = 0;
        double bits = 0;
        for (int v : ordem) {
            K = max(K, tamF + 1);
            proc[v] = 1;
            for (auto [u, w] : g.adj[v]) cnt[u]--;
            vector<int> sai;
            if (cnt[v] > 0) noF[v] = 1;
            tamF++;
            if (cnt[v] == 0) sai.push_back(v);
            for (auto [u, w] : g.adj[v])
                if (noF[u] && cnt[u] == 0) sai.push_back(u), noF[u] = 0;
            int atual = tamF;
            for (size_t i = 0; i < sai.size(); i++) {
                bits += ldexp(1.0, atual - 1);
                atual--;
            }
            tamF = atual;
            if (K > larguraMax) break;
        }
        r.largura = K;
        if (K > larguraMax) {
            r.motivo = "largura " + to_string(K) + " > limite " + to_string(larguraMax);
            return r;
        }
        double mem = bits / 8.0 / 1048576.0 + 3.0 * ldexp(4.0, K) / 1048576.0;
        r.memMB = mem;
        if (mem > memMaxMB) {
            r.motivo = "memoria estimada " + to_string((long long)mem) + " MB > limite";
            return r;
        }
    }

    auto t0 = Clock::now();
    const int NEG = INT_MIN / 4;
    struct Esquecimento {
        int j, vertice;
        vector<uint64_t> bits;  // bit m = 1 se, no estado m (sem o vertice), o vertice estava no lado 1
    };
    vector<vector<Esquecimento>> registro(n);
    vector<int> kAntes(n);
    vector<int> cnt(n);
    for (int v = 0; v < n; v++) cnt[v] = g.adj[v].size();
    vector<char> proc(n, 0);
    vector<int> F, pos(n, -1);
    vector<int> f(1, 0), h;

    for (int t = 0; t < n; t++) {
        int v = ordem[t];
        int k = F.size();
        kAntes[t] = k;
        vector<pair<int, int>> viz;  // (bit na fronteira, peso) dos vizinhos ja processados
        for (auto [u, w] : g.adj[v])
            if (proc[u]) viz.push_back({pos[u], w});
        h.assign((size_t)1 << (k + 1), NEG);
        bool novaComp = (k == 0);  // fronteira vazia: v abre componente nova, simetria => lado 0
        for (size_t mask = 0; mask < ((size_t)1 << k); mask++) {
            ll g0 = 0, g1 = 0;
            for (auto [j, w] : viz) {
                if ((mask >> j) & 1) g0 += w;  // v no lado 0 corta arestas para vizinhos no lado 1
                else g1 += w;                  // v no lado 1 corta arestas para vizinhos no lado 0
            }
            h[mask] = f[mask] + (int)g0;
            if (!novaComp) h[mask | ((size_t)1 << k)] = f[mask] + (int)g1;
        }
        F.push_back(v);
        pos[v] = k;
        proc[v] = 1;
        for (auto [u, w] : g.adj[v]) cnt[u]--;
        // esquece (maximiza sobre) os vertices que nao tem mais vizinhos nao processados, do maior bit ao menor
        for (int j = (int)F.size() - 1; j >= 0; j--) {
            if (cnt[F[j]] != 0) continue;
            int K = F.size();
            size_t metade = (size_t)1 << (K - 1);
            f.assign(metade, 0);
            Esquecimento e;
            e.j = j;
            e.vertice = F[j];
            e.bits.assign((metade + 63) / 64, 0);
            size_t baixo = ((size_t)1 << j) - 1;
            for (size_t m2 = 0; m2 < metade; m2++) {
                size_t m0 = (m2 & baixo) | ((m2 >> j) << (j + 1));
                size_t m1 = m0 | ((size_t)1 << j);
                if (h[m1] > h[m0]) {
                    f[m2] = h[m1];
                    e.bits[m2 >> 6] |= (uint64_t)1 << (m2 & 63);
                } else {
                    f[m2] = h[m0];
                }
            }
            h.swap(f);  // h passa a ser a tabela sem o vertice; f e reutilizado
            F.erase(F.begin() + j);
            for (size_t i = j; i < F.size(); i++) pos[F[i]] = i;
            pos[e.vertice] = -1;
            registro[t].push_back(move(e));
        }
        f.swap(h);
        if (segundos(t0, Clock::now()) > limiteTempo) {
            r.motivo = "limite de tempo";
            r.tempo = segundos(t0, Clock::now());
            return r;
        }
    }
    r.valor = f[0];

    // --- reconstrucao da particao otima ---
    r.lado.assign(n, 0);
    uint64_t mask = 0;
    for (int t = n - 1; t >= 0; t--) {
        for (int i = (int)registro[t].size() - 1; i >= 0; i--) {
            auto &e = registro[t][i];
            int b = (e.bits[mask >> 6] >> (mask & 63)) & 1;
            r.lado[e.vertice] = b;
            uint64_t baixo = mask & (((uint64_t)1 << e.j) - 1), alto = mask >> e.j;
            mask = baixo | ((uint64_t)b << e.j) | (alto << (e.j + 1));
        }
        int k = kAntes[t];
        uint64_t baixo = mask & (((uint64_t)1 << k) - 1), alto = mask >> (k + 1);
        mask = baixo | (alto << k);
    }
    r.ok = true;
    r.tempo = segundos(t0, Clock::now());
    return r;
}

//  Interface

#ifndef SEM_MAIN
namespace fs = std::filesystem;

static string particaoCompacta(const vector<int> &lado) {
    string s = "Particao (lado de cada vertice, 1..n):\n";
    for (size_t i = 0; i < lado.size(); i++) {
        s += char('0' + lado[i]);
        if ((i + 1) % 100 == 0 || i + 1 == lado.size()) s += '\n';
    }
    return s;
}

int main(int argc, char **argv) {
    string instancia, saida, algoritmo = "ambos", csv;
    double tempoMax = 60;
    int larguraMax = 26;
    double memMax = 2000;
    bool soLargura = false;
    vector<string> pos;
    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        auto prox = [&]() -> string { return i + 1 < argc ? argv[++i] : ""; };
        if (a == "--algoritmo") algoritmo = prox();
        else if (a == "--tempo") tempoMax = atof(prox().c_str());
        else if (a == "--largura-max") larguraMax = atoi(prox().c_str());
        else if (a == "--memoria-max") memMax = atof(prox().c_str());
        else if (a == "--csv") csv = prox();
        else if (a == "--so-largura") soLargura = true;
        else pos.push_back(a);
    }
    if (algoritmo != "bb" && algoritmo != "dp" && algoritmo != "ambos") {
        cerr << "--algoritmo deve ser bb, dp ou ambos\n";
        return 1;
    }
    if (!pos.empty()) instancia = pos[0];
    if (pos.size() > 1) saida = pos[1];

    if (instancia.empty()) {  // menu: escolhe o caso de teste dentre os arquivos de instancias_t2/
        vector<string> arquivos;
        for (const char *pasta : {"instancias_t2", "instancias_t1"})  // casos novos (T2) e instancias do Trabalho 1
            if (fs::exists(pasta))
                for (auto &e : fs::directory_iterator(pasta))
                    if (e.is_regular_file()) arquivos.push_back(e.path().string());
        sort(arquivos.begin(), arquivos.end());
        if (arquivos.empty()) {
            cerr << "Uso: " << argv[0] << " <instancia> [saida] [opcoes]   (pasta instancias_t2/ nao encontrada)\n";
            return 1;
        }
        cout << "Instancias disponiveis:\n";
        for (size_t i = 0; i < arquivos.size(); i++) cout << "  [" << i + 1 << "] " << arquivos[i] << "\n";
        cout << "Escolha o numero: ";
        size_t k;
        if (!(cin >> k) || k < 1 || k > arquivos.size()) {
            cerr << "Escolha invalida\n";
            return 1;
        }
        instancia = arquivos[k - 1];
    }

    Grafo g;
    if (!lerGrafo(instancia, g)) {
        cerr << "Nao foi possivel ler a instancia '" << instancia << "'\n";
        return 1;
    }
    string nome = fs::path(instancia).filename().string();

    if (soLargura) {  // apenas estima a largura da ordem (para saber se a DP e viavel)
        auto o = melhorOrdemFronteira(g, 5, larguraMax, min(tempoMax, 30.0));
        cout << nome << " n=" << g.n << " m=" << g.m << " largura_estimada=";
        if (o.largura == INT_MAX) cout << ">" << larguraMax;
        else cout << o.largura;
        cout << " (" << o.tempo << " s)\n";
        return 0;
    }

    ostringstream r;
    r << fixed << setprecision(4);
    r << "Instancia: " << instancia << "\nn=" << g.n << " m=" << g.m << " peso_total=" << g.pesoTotal << "\n\n";

    // ---- heuristica do Trabalho 1 ----
    auto t0 = Clock::now();
    vector<int> ladoH = heuristicaT1(g);
    double tempoH = segundos(t0, Clock::now());
    ll valH = valorCorte(g, ladoH);
    r << "[Heuristica T1] guloso+busca local: " << valH << " (" << tempoH << " s)\n";

    ll melhor = valH;
    vector<int> ladoMelhor = ladoH;
    bool provado = false;
    string quemProvou;

    // ---- Algoritmo 1: Branch-and-Bound ----
    ResultadoBB bb;
    bool rodouBB = false;
    if (algoritmo == "bb" || algoritmo == "ambos") {
        if (g.n > 5000) {
            r << "[Branch-and-Bound] nao executado (n > 5000)\n";
        } else {
            rodouBB = true;
            bb = branchAndBound(g, ladoH, tempoMax);
            r << "[Branch-and-Bound] valor=" << bb.valor << (bb.otimo ? " (OTIMO provado)" : " (limite de tempo: otimalidade NAO provada)")
              << " nos=" << bb.nos << " tempo=" << bb.tempo << " s\n";
            if (bb.valor > melhor) melhor = bb.valor, ladoMelhor = bb.lado;
            if (bb.otimo) provado = true, quemProvou = "Branch-and-Bound";
        }
    }

    // ---- Algoritmo 2: DP sobre a largura de caminho ----
    ResultadoDP dp;
    bool rodouDP = false;
    if (algoritmo == "dp" || algoritmo == "ambos") {
        auto o = melhorOrdemFronteira(g, 30, larguraMax, min(tempoMax, 20.0));
        if (o.largura == INT_MAX) {
            r << "[DP largura] inviavel: nao foi encontrada ordem com largura <= " << larguraMax << " (busca: " << o.tempo << " s)\n";
        } else {
            rodouDP = true;
            dp = dpLargura(g, o.ordem, larguraMax, tempoMax, memMax);
            if (dp.ok) {
                r << "[DP largura] largura=" << dp.largura << " valor=" << dp.valor << " (OTIMO) tempo=" << dp.tempo << " s (ordem: "
                  << o.tempo << " s) memoria~" << (long long)dp.memMB << " MB\n";
                if (dp.valor >= melhor) melhor = dp.valor, ladoMelhor = dp.lado;
                provado = true;
                quemProvou = quemProvou.empty() ? "DP largura" : quemProvou + " e DP largura";
            } else {
                r << "[DP largura] largura=" << dp.largura << " nao executada: " << dp.motivo << "\n";
            }
        }
    }

    // ---- conferencia ----
    ll conferido = valorCorte(g, ladoMelhor);
    r << "\n";
    if (provado) {
        r << "Valor otimo: " << melhor << " (provado por " << quemProvou << ")\n";
        r << "Heuristica T1 / otimo: " << (melhor != 0 ? (double)valH / melhor : 0.0) << "\n";
    } else {
        r << "Melhor valor encontrado: " << melhor << " (otimalidade NAO provada)\n";
    }
    bool consistente = true;
    if (rodouBB && bb.otimo && rodouDP && dp.ok && bb.valor != dp.valor) consistente = false;
    if (rodouBB && rodouDP && dp.ok && bb.otimo && bb.valor != dp.valor) r << "ERRO: algoritmos discordam!\n";
    r << "Conferencia: valor recalculado da particao final = " << conferido << (conferido == melhor && consistente ? " (OK)\n" : " (FALHA)\n");
    r << "\n" << particaoCompacta(ladoMelhor);

    cout << r.str();
    if (saida.empty()) {
        fs::create_directories("resultados_t2");
        saida = "resultados_t2/" + nome + ".txt";
    }
    {
        ofstream f(saida);
        // o arquivo guarda o relatorio e a particao no formato "vertice lado" (igual ao do Trabalho 1)
        string rel = r.str();
        rel = rel.substr(0, rel.find("\nParticao (lado de cada vertice"));
        f << rel << "\nParticao (vertice: lado):\n";
        for (int v = 0; v < g.n; v++) f << v + 1 << ' ' << ladoMelhor[v] << '\n';
    }
    cout << "Relatorio e solucao salvos em " << saida << endl;

    if (!csv.empty()) {  // linha para a tabela de experimentos
        bool novo = !fs::exists(csv);
        ofstream f(csv, ios::app);
        if (novo) f << "instancia,n,m,peso_total,heur,t_heur,bb_valor,bb_otimo,bb_nos,bb_tempo,dp_largura,dp_ok,dp_valor,dp_tempo,dp_mem_mb,otimo\n";
        f << nome << ',' << g.n << ',' << g.m << ',' << g.pesoTotal << ',' << valH << ',' << tempoH << ',';
        if (rodouBB) f << bb.valor << ',' << bb.otimo << ',' << bb.nos << ',' << bb.tempo << ',';
        else f << ",,,,";
        if (rodouDP) f << dp.largura << ',' << dp.ok << ',' << (dp.ok ? dp.valor : 0) << ',' << dp.tempo << ',' << (long long)dp.memMB << ',';
        else f << ",,,,,";
        f << (provado ? to_string(melhor) : "") << '\n';
    }
    return 0;
}
#endif
