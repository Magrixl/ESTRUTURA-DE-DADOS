#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <thread>
#include<limits>

using namespace std;

// ===============================
// REGISTRO PRINCIPAL DO DATASET
// ===============================

struct RegistroGPS {
    uint64_t timestamp;
    double latitude;
    double longitude;
    float elevacao;
    float velocidade;
};

// ===============================
// VALIDACAO DOS REGISTROS
// ===============================

bool registroValido(const RegistroGPS& r) {
    if (r.latitude < -90.0 || r.latitude > 90.0) return false;
    if (r.longitude < -180.0 || r.longitude > 180.0) return false;

    // Faixa aproximada para aplicacao GPS terrestre
    if (r.elevacao < -500.0f || r.elevacao > 9000.0f) return false;

    // Velocidade negativa nao faz sentido neste contexto
    if (r.velocidade < 0.0f) return false;

    // Evita NaN ou infinito
    if (!isfinite(r.latitude)) return false;
    if (!isfinite(r.longitude)) return false;
    if (!isfinite(r.elevacao)) return false;
    if (!isfinite(r.velocidade)) return false;

    return true;
}

// ===============================
// LEITURA DO CSV
// Formato esperado:
// timestamp,latitude,longitude,elevacao,velocidade
// ===============================

vector<RegistroGPS> carregarDados(const string& nomeArquivo) {
    vector<RegistroGPS> dados;
    ifstream arquivo(nomeArquivo);

    if (!arquivo.is_open()) {
        cerr << "Erro: nao foi possivel abrir o arquivo: " << nomeArquivo << endl;
        return dados;
    }

    string linha;

    // Ignora o cabecalho
    getline(arquivo, linha);

    int linhasLidas = 0;
    int linhasInvalidas = 0;

    while (getline(arquivo, linha)) {
        linhasLidas++;

        stringstream ss(linha);
        string item;
        RegistroGPS reg;

        try {
            getline(ss, item, ',');
            reg.timestamp = stoull(item);

            getline(ss, item, ',');
            reg.latitude = stod(item);

            getline(ss, item, ',');
            reg.longitude = stod(item);

            getline(ss, item, ',');
            reg.elevacao = stof(item);

            getline(ss, item, ',');
            reg.velocidade = stof(item);

            if (registroValido(reg)) {
                dados.push_back(reg);
            } else {
                linhasInvalidas++;
            }
        } catch (...) {
            linhasInvalidas++;
        }
    }

    arquivo.close();

    cout << "Linhas lidas do CSV:       " << linhasLidas << endl;
    cout << "Registros validos:         " << dados.size() << endl;
    cout << "Registros descartados:     " << linhasInvalidas << endl;

    return dados;
}

// ===============================
// TRATAMENTO INICIAL DOS DADOS
// ===============================

void ordenarPorTimestamp(vector<RegistroGPS>& dados) {
    sort(dados.begin(), dados.end(), [](const RegistroGPS& a, const RegistroGPS& b) {
        return a.timestamp < b.timestamp;
    });
}

void removerTimestampsDuplicados(vector<RegistroGPS>& dados) {
    if (dados.empty()) return;

    auto novoFim = unique(dados.begin(), dados.end(), [](const RegistroGPS& a, const RegistroGPS& b) {
        return a.timestamp == b.timestamp;
    });

    int removidos = static_cast<int>(distance(novoFim, dados.end()));
    dados.erase(novoFim, dados.end());

    cout << "Timestamps duplicados removidos: " << removidos << endl;
}

// ===============================
// ESTATISTICAS BASICAS
// ===============================

float calcularVelocidadeMedia(const vector<RegistroGPS>& dados) {
    if (dados.empty()) return 0.0f;

    double soma = 0.0;

    for (const auto& r : dados) {
        soma += r.velocidade;
    }

    return static_cast<float>(soma / dados.size());
}

float calcularVelocidadeMaxima(const vector<RegistroGPS>& dados) {
    if (dados.empty()) return 0.0f;

    float maior = dados[0].velocidade;

    for (const auto& r : dados) {
        if (r.velocidade > maior) {
            maior = r.velocidade;
        }
    }

    return maior;
}

float calcularVelocidadeMinima(const vector<RegistroGPS>& dados) {
    if (dados.empty()) return 0.0f;

    float menor = dados[0].velocidade;

    for (const auto& r : dados) {
        if (r.velocidade < menor) {
            menor = r.velocidade;
        }
    }

    return menor;
}

// ===============================
// VISUALIZACAO DO STATUS
// ===============================

void exibirStatusSistema(const vector<RegistroGPS>& buffer) {
    cout << "\n========================================" << endl;
    cout << "   [STATUS DO SISTEMA DE TELEMETRIA]    " << endl;
    cout << "========================================" << endl;

    cout << "Registros carregados:      " << buffer.size() << endl;
    cout << "Memoria estimada do vetor: "
         << (buffer.size() * sizeof(RegistroGPS)) / 1024.0
         << " KB" << endl;

    if (!buffer.empty()) {
        cout << "Timestamp inicial:         " << buffer.front().timestamp << endl;
        cout << "Timestamp final:           " << buffer.back().timestamp << endl;
        cout << "Velocidade media:          " << calcularVelocidadeMedia(buffer) << endl;
        cout << "Velocidade minima:         " << calcularVelocidadeMinima(buffer) << endl;
        cout << "Velocidade maxima:         " << calcularVelocidadeMaxima(buffer) << endl;
    }

    cout << "Status do dataset:         Integridade inicial verificada" << endl;
    cout << "========================================\n" << endl;
}

// ===============================
// ARVORE AVL
// Estrutura tradicional usada para indexar registros por timestamp
// Operacoes implementadas:
// inserir, buscar e remover
// ===============================

struct AVLNode {
    uint64_t timestamp;
    int indice;
    int altura;
    AVLNode* esq;
    AVLNode* dir;

    AVLNode(uint64_t ts, int idx) {
        timestamp = ts;
        indice = idx;
        altura = 1;
        esq = nullptr;
        dir = nullptr;
    }
};

class ArvoreAVL {
private:
    AVLNode* raiz;
    int totalNos;

    int altura(AVLNode* no) const {
        return no ? no->altura : 0;
    }

    int fatorBalanceamento(AVLNode* no) const {
        return no ? altura(no->esq) - altura(no->dir) : 0;
    }

    void atualizarAltura(AVLNode* no) {
        if (no) {
            no->altura = 1 + max(altura(no->esq), altura(no->dir));
        }
    }

    AVLNode* rotacaoDireita(AVLNode* y) {
        AVLNode* x = y->esq;
        AVLNode* t2 = x->dir;

        x->dir = y;
        y->esq = t2;

        atualizarAltura(y);
        atualizarAltura(x);

        return x;
    }

    AVLNode* rotacaoEsquerda(AVLNode* x) {
        AVLNode* y = x->dir;
        AVLNode* t2 = y->esq;

        y->esq = x;
        x->dir = t2;

        atualizarAltura(x);
        atualizarAltura(y);

        return y;
    }

    AVLNode* balancear(AVLNode* no) {
        if (!no) return no;

        atualizarAltura(no);

        int fb = fatorBalanceamento(no);

        // Caso esquerda-esquerda
        if (fb > 1 && fatorBalanceamento(no->esq) >= 0) {
            return rotacaoDireita(no);
        }

        // Caso esquerda-direita
        if (fb > 1 && fatorBalanceamento(no->esq) < 0) {
            no->esq = rotacaoEsquerda(no->esq);
            return rotacaoDireita(no);
        }

        // Caso direita-direita
        if (fb < -1 && fatorBalanceamento(no->dir) <= 0) {
            return rotacaoEsquerda(no);
        }

        // Caso direita-esquerda
        if (fb < -1 && fatorBalanceamento(no->dir) > 0) {
            no->dir = rotacaoDireita(no->dir);
            return rotacaoEsquerda(no);
        }

        return no;
    }

    AVLNode* inserirRec(AVLNode* no, uint64_t timestamp, int indice, bool& inseriu) {
        if (!no) {
            inseriu = true;
            totalNos++;
            return new AVLNode(timestamp, indice);
        }

        if (timestamp < no->timestamp) {
            no->esq = inserirRec(no->esq, timestamp, indice, inseriu);
        } else if (timestamp > no->timestamp) {
            no->dir = inserirRec(no->dir, timestamp, indice, inseriu);
        } else {
            // Se o timestamp ja existe, atualiza o indice
            no->indice = indice;
            inseriu = false;
            return no;
        }

        return balancear(no);
    }

    AVLNode* menorValorNo(AVLNode* no) {
        AVLNode* atual = no;

        while (atual && atual->esq != nullptr) {
            atual = atual->esq;
        }

        return atual;
    }

    AVLNode* removerRec(AVLNode* no, uint64_t timestamp, bool& removeu) {
        if (!no) return nullptr;

        if (timestamp < no->timestamp) {
            no->esq = removerRec(no->esq, timestamp, removeu);
        } else if (timestamp > no->timestamp) {
            no->dir = removerRec(no->dir, timestamp, removeu);
        } else {
            removeu = true;

            // Caso 1: no sem filho ou com apenas um filho
            if (!no->esq || !no->dir) {
                AVLNode* filho = no->esq ? no->esq : no->dir;

                if (!filho) {
                    delete no;
                    totalNos--;
                    return nullptr;
                }

                AVLNode* antigo = no;
                no = filho;
                delete antigo;
                totalNos--;
            }

            // Caso 2: no com dois filhos
            else {
                AVLNode* sucessor = menorValorNo(no->dir);

                no->timestamp = sucessor->timestamp;
                no->indice = sucessor->indice;

                bool removeuSucessor = false;
                no->dir = removerRec(no->dir, sucessor->timestamp, removeuSucessor);
            }
        }

        if (!no) return nullptr;

        return balancear(no);
    }

    int buscarRec(AVLNode* no, uint64_t timestamp) const {
        if (!no) return -1;

        if (timestamp == no->timestamp) {
            return no->indice;
        }

        if (timestamp < no->timestamp) {
            return buscarRec(no->esq, timestamp);
        }

        return buscarRec(no->dir, timestamp);
    }

    void destruirRec(AVLNode* no) {
        if (!no) return;

        destruirRec(no->esq);
        destruirRec(no->dir);

        delete no;
    }

    bool verificarBalanceamentoRec(AVLNode* no) const {
        if (!no) return true;

        int fb = fatorBalanceamento(no);

        if (fb < -1 || fb > 1) {
            return false;
        }

        return verificarBalanceamentoRec(no->esq) &&
               verificarBalanceamentoRec(no->dir);
    }

public:
    ArvoreAVL() {
        raiz = nullptr;
        totalNos = 0;
    }

    ~ArvoreAVL() {
        destruirRec(raiz);
    }

    // Evita copia acidental da arvore, pois ela usa ponteiros dinamicos
    ArvoreAVL(const ArvoreAVL&) = delete;
    ArvoreAVL& operator=(const ArvoreAVL&) = delete;

    bool inserir(uint64_t timestamp, int indice) {
        bool inseriu = false;
        raiz = inserirRec(raiz, timestamp, indice, inseriu);
        return inseriu;
    }

    bool remover(uint64_t timestamp) {
        bool removeu = false;
        raiz = removerRec(raiz, timestamp, removeu);
        return removeu;
    }

    int buscar(uint64_t timestamp) const {
        return buscarRec(raiz, timestamp);
    }

    int tamanho() const {
        return totalNos;
    }

    int alturaArvore() const {
        return altura(raiz);
    }

    bool estaBalanceada() const {
        return verificarBalanceamentoRec(raiz);
    }

    double memoriaEstimadaKB() const {
        return (totalNos * sizeof(AVLNode)) / 1024.0;
    }
};
// ===============================
// LISTA ENCADEADA
// Estrutura tradicional usada para simular fluxo sequencial de registros GPS
// Operacoes implementadas:
// inserir, buscar e remover
// ===============================

struct ListaNode {
    uint64_t timestamp;
    int indice;
    ListaNode* prox;

    ListaNode(uint64_t ts, int idx) {
        timestamp = ts;
        indice = idx;
        prox = nullptr;
    }
};

class ListaEncadeada {
private:
    ListaNode* inicio;
    ListaNode* fim;
    int totalNos;

public:
    ListaEncadeada() {
        inicio = nullptr;
        fim = nullptr;
        totalNos = 0;
    }

    ~ListaEncadeada() {
        limpar();
    }

    ListaEncadeada(const ListaEncadeada&) = delete;
    ListaEncadeada& operator=(const ListaEncadeada&) = delete;

    void inserirFim(uint64_t timestamp, int indice) {
        ListaNode* novo = new ListaNode(timestamp, indice);

        if (!inicio) {
            inicio = novo;
            fim = novo;
        } else {
            fim->prox = novo;
            fim = novo;
        }

        totalNos++;
    }

    int buscar(uint64_t timestamp) const {
        ListaNode* atual = inicio;

        while (atual) {
            if (atual->timestamp == timestamp) {
                return atual->indice;
            }

            atual = atual->prox;
        }

        return -1;
    }

    bool remover(uint64_t timestamp) {
        if (!inicio) return false;

        ListaNode* atual = inicio;
        ListaNode* anterior = nullptr;

        while (atual) {
            if (atual->timestamp == timestamp) {
                if (anterior) {
                    anterior->prox = atual->prox;
                } else {
                    inicio = atual->prox;
                }

                if (atual == fim) {
                    fim = anterior;
                }

                delete atual;
                totalNos--;

                return true;
            }

            anterior = atual;
            atual = atual->prox;
        }

        return false;
    }

    void limpar() {
        ListaNode* atual = inicio;

        while (atual) {
            ListaNode* proximo = atual->prox;
            delete atual;
            atual = proximo;
        }

        inicio = nullptr;
        fim = nullptr;
        totalNos = 0;
    }

    int tamanho() const {
        return totalNos;
    }

    bool vazia() const {
        return totalNos == 0;
    }

    double memoriaEstimadaKB() const {
        return (totalNos * sizeof(ListaNode)) / 1024.0;
    }
};
// ===============================
// TABELA HASH
// Estrutura tradicional usada para busca rapida por timestamp
// Implementacao com encadeamento separado
// Operacoes implementadas:
// inserir, buscar e remover
// Tambem mede colisoes
// ===============================

struct HashNode {
    uint64_t timestamp;
    int indice;
    HashNode* prox;

    HashNode(uint64_t ts, int idx) {
        timestamp = ts;
        indice = idx;
        prox = nullptr;
    }
};

class TabelaHash {
private:
    vector<HashNode*> tabela;
    int capacidade;
    int totalNos;
    long long totalColisoes;

    int funcaoHash(uint64_t timestamp) const {
        uint64_t misturado = timestamp ^ (timestamp >> 32);
        return static_cast<int>(misturado % capacidade);
    }

public:
    TabelaHash(int cap) {
        capacidade = max(101, cap);
        tabela.assign(capacidade, nullptr);
        totalNos = 0;
        totalColisoes = 0;
    }

    ~TabelaHash() {
        limpar();
    }

    TabelaHash(const TabelaHash&) = delete;
    TabelaHash& operator=(const TabelaHash&) = delete;

    bool inserir(uint64_t timestamp, int indice) {
        int pos = funcaoHash(timestamp);

        HashNode* atual = tabela[pos];

        // Se a chave ja existe, apenas atualiza o indice
        while (atual) {
            if (atual->timestamp == timestamp) {
                atual->indice = indice;
                return false;
            }

            atual = atual->prox;
        }

        // Se o balde ja possuia algum elemento, houve colisao
        if (tabela[pos] != nullptr) {
            totalColisoes++;
        }

        HashNode* novo = new HashNode(timestamp, indice);
        novo->prox = tabela[pos];
        tabela[pos] = novo;

        totalNos++;

        return true;
    }

    int buscar(uint64_t timestamp) const {
        int pos = funcaoHash(timestamp);

        HashNode* atual = tabela[pos];

        while (atual) {
            if (atual->timestamp == timestamp) {
                return atual->indice;
            }

            atual = atual->prox;
        }

        return -1;
    }

    bool remover(uint64_t timestamp) {
        int pos = funcaoHash(timestamp);

        HashNode* atual = tabela[pos];
        HashNode* anterior = nullptr;

        while (atual) {
            if (atual->timestamp == timestamp) {
                if (anterior) {
                    anterior->prox = atual->prox;
                } else {
                    tabela[pos] = atual->prox;
                }

                delete atual;
                totalNos--;

                return true;
            }

            anterior = atual;
            atual = atual->prox;
        }

        return false;
    }

    void limpar() {
        for (int i = 0; i < capacidade; i++) {
            HashNode* atual = tabela[i];

            while (atual) {
                HashNode* proximo = atual->prox;
                delete atual;
                atual = proximo;
            }

            tabela[i] = nullptr;
        }

        totalNos = 0;
        totalColisoes = 0;
    }

    int tamanho() const {
        return totalNos;
    }

    int getCapacidade() const {
        return capacidade;
    }

    long long colisoes() const {
        return totalColisoes;
    }

    double fatorCarga() const {
        return static_cast<double>(totalNos) / capacidade;
    }

    double memoriaEstimadaKB() const {
        double memoriaNos = totalNos * sizeof(HashNode);
        double memoriaTabela = capacidade * sizeof(HashNode*);
        return (memoriaNos + memoriaTabela) / 1024.0;
    }
};

// ===============================
// MAIN
// ===============================

// ===============================
// FENWICK TREE / BINARY INDEXED TREE
// Estrutura fora da ementa
// Usada para consultas eficientes de soma prefixada
// Neste projeto: soma de ganho de elevacao em intervalos
// ===============================

class FenwickTree {
private:
    vector<double> arvore;
    vector<double> valores;
    int n;

    void adicionarInterno(int indice1Baseado, double delta) {
        for (int i = indice1Baseado; i <= n; i += (i & -i)) {
            arvore[i] += delta;
        }
    }

public:
    FenwickTree(int tamanho) {
        n = tamanho;
        arvore.assign(n + 1, 0.0);
        valores.assign(n, 0.0);
    }

    // Insere ou atualiza um valor em um indice 0-baseado
    void atualizar(int indice, double novoValor) {
        if (indice < 0 || indice >= n) return;

        double delta = novoValor - valores[indice];
        valores[indice] = novoValor;

        adicionarInterno(indice + 1, delta);
    }

    // Remove logicamente o valor de uma posicao
    bool remover(int indice) {
        if (indice < 0 || indice >= n) return false;

        atualizar(indice, 0.0);
        return true;
    }

    // Retorna o valor armazenado exatamente naquele indice
    double buscarValor(int indice) const {
        if (indice < 0 || indice >= n) return 0.0;

        return valores[indice];
    }

    // Soma do inicio ate o indice informado
    double somaPrefixo(int indice) const {
        if (indice < 0) return 0.0;
        if (indice >= n) indice = n - 1;

        double soma = 0.0;
        int i = indice + 1;

        while (i > 0) {
            soma += arvore[i];
            i -= (i & -i);
        }

        return soma;
    }

    // Soma entre dois indices 0-baseados
    double somaIntervalo(int esquerda, int direita) const {
        if (esquerda > direita) return 0.0;
        if (direita < 0 || esquerda >= n) return 0.0;

        if (esquerda < 0) esquerda = 0;
        if (direita >= n) direita = n - 1;

        if (esquerda == 0) {
            return somaPrefixo(direita);
        }

        return somaPrefixo(direita) - somaPrefixo(esquerda - 1);
    }

    int tamanho() const {
        return n;
    }

    double memoriaEstimadaKB() const {
        double memoriaArvore = arvore.size() * sizeof(double);
        double memoriaValores = valores.size() * sizeof(double);

        return (memoriaArvore + memoriaValores) / 1024.0;
    }
};

// ===============================
// KD-TREE
// Estrutura fora da ementa
// Usada para busca espacial por latitude e longitude
// Operacoes implementadas:
// inserir, buscar ponto mais proximo e remover logicamente
// ===============================

struct KDNode {
    double latitude;
    double longitude;
    uint64_t timestamp;
    int indice;
    bool ativo;
    KDNode* esq;
    KDNode* dir;

    KDNode(double lat, double lon, uint64_t ts, int idx) {
        latitude = lat;
        longitude = lon;
        timestamp = ts;
        indice = idx;
        ativo = true;
        esq = nullptr;
        dir = nullptr;
    }
};

class KDTree {
private:
    KDNode* raiz;
    int totalNos;
    int totalAtivos;

    double distanciaQuadrada(double lat1, double lon1, double lat2, double lon2) const {
        double dLat = lat1 - lat2;
        double dLon = lon1 - lon2;
        return dLat * dLat + dLon * dLon;
    }

    KDNode* inserirRec(KDNode* no, double latitude, double longitude, uint64_t timestamp, int indice, int profundidade) {
        if (!no) {
            totalNos++;
            totalAtivos++;
            return new KDNode(latitude, longitude, timestamp, indice);
        }

        int dimensao = profundidade % 2;

        if (dimensao == 0) {
            if (latitude < no->latitude) {
                no->esq = inserirRec(no->esq, latitude, longitude, timestamp, indice, profundidade + 1);
            } else {
                no->dir = inserirRec(no->dir, latitude, longitude, timestamp, indice, profundidade + 1);
            }
        } else {
            if (longitude < no->longitude) {
                no->esq = inserirRec(no->esq, latitude, longitude, timestamp, indice, profundidade + 1);
            } else {
                no->dir = inserirRec(no->dir, latitude, longitude, timestamp, indice, profundidade + 1);
            }
        }

        return no;
    }

    void buscarMaisProximoRec(
        KDNode* no,
        double latitudeAlvo,
        double longitudeAlvo,
        int profundidade,
        KDNode*& melhor,
        double& melhorDistancia
    ) const {
        if (!no) return;

        double distanciaAtual = distanciaQuadrada(
            latitudeAlvo,
            longitudeAlvo,
            no->latitude,
            no->longitude
        );

        if (no->ativo && distanciaAtual < melhorDistancia) {
            melhorDistancia = distanciaAtual;
            melhor = no;
        }

        int dimensao = profundidade % 2;

        KDNode* primeiro;
        KDNode* segundo;

        double diferenca;

        if (dimensao == 0) {
            diferenca = latitudeAlvo - no->latitude;

            if (latitudeAlvo < no->latitude) {
                primeiro = no->esq;
                segundo = no->dir;
            } else {
                primeiro = no->dir;
                segundo = no->esq;
            }
        } else {
            diferenca = longitudeAlvo - no->longitude;

            if (longitudeAlvo < no->longitude) {
                primeiro = no->esq;
                segundo = no->dir;
            } else {
                primeiro = no->dir;
                segundo = no->esq;
            }
        }

        buscarMaisProximoRec(primeiro, latitudeAlvo, longitudeAlvo, profundidade + 1, melhor, melhorDistancia);

        if (diferenca * diferenca < melhorDistancia) {
            buscarMaisProximoRec(segundo, latitudeAlvo, longitudeAlvo, profundidade + 1, melhor, melhorDistancia);
        }
    }

    bool removerPorTimestampRec(KDNode* no, uint64_t timestamp) {
        if (!no) return false;

        if (no->timestamp == timestamp && no->ativo) {
            no->ativo = false;
            totalAtivos--;
            return true;
        }

        if (removerPorTimestampRec(no->esq, timestamp)) {
            return true;
        }

        return removerPorTimestampRec(no->dir, timestamp);
    }

    void destruirRec(KDNode* no) {
        if (!no) return;

        destruirRec(no->esq);
        destruirRec(no->dir);

        delete no;
    }

    int alturaRec(KDNode* no) const {
        if (!no) return 0;

        int alturaEsq = alturaRec(no->esq);
        int alturaDir = alturaRec(no->dir);

        return 1 + max(alturaEsq, alturaDir);
    }

public:
    KDTree() {
        raiz = nullptr;
        totalNos = 0;
        totalAtivos = 0;
    }

    ~KDTree() {
        destruirRec(raiz);
    }

    KDTree(const KDTree&) = delete;
    KDTree& operator=(const KDTree&) = delete;

    void inserir(double latitude, double longitude, uint64_t timestamp, int indice) {
        raiz = inserirRec(raiz, latitude, longitude, timestamp, indice, 0);
    }

    int buscarMaisProximo(double latitude, double longitude) const {
        if (!raiz || totalAtivos == 0) {
            return -1;
        }

        KDNode* melhor = nullptr;
        double melhorDistancia = numeric_limits<double>::max();

        buscarMaisProximoRec(raiz, latitude, longitude, 0, melhor, melhorDistancia);

        if (!melhor) {
            return -1;
        }

        return melhor->indice;
    }

    bool removerPorTimestamp(uint64_t timestamp) {
        return removerPorTimestampRec(raiz, timestamp);
    }

    int tamanhoTotal() const {
        return totalNos;
    }

    int tamanhoAtivo() const {
        return totalAtivos;
    }

    int alturaArvore() const {
        return alturaRec(raiz);
    }

    double memoriaEstimadaKB() const {
        return (totalNos * sizeof(KDNode)) / 1024.0;
    }
};

// ===============================
// OPERACOES ADICIONAIS SOBRE OS DADOS
// Exigencia do projeto:
// 1. Estatisticas
// 2. Filtragem / deteccao de anomalias
// 3. Classificacao de movimento
// ===============================

struct EstatisticasTelemetria {
    double mediaVelocidade;
    double desvioVelocidade;
    double menorVelocidade;
    double maiorVelocidade;

    double mediaElevacao;
    double desvioElevacao;
    double menorElevacao;
    double maiorElevacao;
};

EstatisticasTelemetria calcularEstatisticasCompletas(const vector<RegistroGPS>& dados) {
    EstatisticasTelemetria est{};

    if (dados.empty()) {
        return est;
    }

    double somaVel = 0.0;
    double somaElev = 0.0;

    est.menorVelocidade = dados[0].velocidade;
    est.maiorVelocidade = dados[0].velocidade;
    est.menorElevacao = dados[0].elevacao;
    est.maiorElevacao = dados[0].elevacao;

    for (const auto& r : dados) {
        somaVel += r.velocidade;
        somaElev += r.elevacao;

        if (r.velocidade < est.menorVelocidade) {
            est.menorVelocidade = r.velocidade;
        }

        if (r.velocidade > est.maiorVelocidade) {
            est.maiorVelocidade = r.velocidade;
        }

        if (r.elevacao < est.menorElevacao) {
            est.menorElevacao = r.elevacao;
        }

        if (r.elevacao > est.maiorElevacao) {
            est.maiorElevacao = r.elevacao;
        }
    }

    est.mediaVelocidade = somaVel / dados.size();
    est.mediaElevacao = somaElev / dados.size();

    double somaQuadradosVel = 0.0;
    double somaQuadradosElev = 0.0;

    for (const auto& r : dados) {
        double diffVel = r.velocidade - est.mediaVelocidade;
        double diffElev = r.elevacao - est.mediaElevacao;

        somaQuadradosVel += diffVel * diffVel;
        somaQuadradosElev += diffElev * diffElev;
    }

    est.desvioVelocidade = sqrt(somaQuadradosVel / dados.size());
    est.desvioElevacao = sqrt(somaQuadradosElev / dados.size());

    return est;
}

struct AnomaliaGPS {
    int indice;
    uint64_t timestamp;
    string motivo;
    float velocidade;
    float elevacao;
};

vector<AnomaliaGPS> detectarAnomalias(
    const vector<RegistroGPS>& dados,
    float limiteVelocidade,
    float limiteSaltoElevacao
) {
    vector<AnomaliaGPS> anomalias;

    for (int i = 0; i < static_cast<int>(dados.size()); i++) {
        const RegistroGPS& atual = dados[i];

        if (atual.velocidade > limiteVelocidade) {
            AnomaliaGPS a;
            a.indice = i;
            a.timestamp = atual.timestamp;
            a.motivo = "Velocidade acima do limite";
            a.velocidade = atual.velocidade;
            a.elevacao = atual.elevacao;

            anomalias.push_back(a);
        }

        if (i > 0) {
            float saltoElevacao = fabs(atual.elevacao - dados[i - 1].elevacao);

            if (saltoElevacao > limiteSaltoElevacao) {
                AnomaliaGPS a;
                a.indice = i;
                a.timestamp = atual.timestamp;
                a.motivo = "Salto brusco de elevacao";
                a.velocidade = atual.velocidade;
                a.elevacao = atual.elevacao;

                anomalias.push_back(a);
            }
        }
    }

    return anomalias;
}

vector<RegistroGPS> filtrarRegistrosPorVelocidade(
    const vector<RegistroGPS>& dados,
    float velocidadeMaximaPermitida
) {
    vector<RegistroGPS> filtrados;

    for (const auto& r : dados) {
        if (r.velocidade <= velocidadeMaximaPermitida) {
            filtrados.push_back(r);
        }
    }

    return filtrados;
}

string classificarMovimento(float velocidade) {
    if (velocidade < 1.0f) {
        return "Parado";
    }

    if (velocidade < 10.0f) {
        return "Lento";
    }

    if (velocidade < 30.0f) {
        return "Moderado";
    }

    return "Rapido";
}

struct ClassificacaoMovimento {
    int parado;
    int lento;
    int moderado;
    int rapido;
};

ClassificacaoMovimento contarClassificacoes(const vector<RegistroGPS>& dados) {
    ClassificacaoMovimento c{0, 0, 0, 0};

    for (const auto& r : dados) {
        string classe = classificarMovimento(r.velocidade);

        if (classe == "Parado") {
            c.parado++;
        } else if (classe == "Lento") {
            c.lento++;
        } else if (classe == "Moderado") {
            c.moderado++;
        } else {
            c.rapido++;
        }
    }

    return c;
}

void exibirOperacoesAdicionais(const vector<RegistroGPS>& dados) {
    cout << "\n========================================" << endl;
    cout << "       [OPERACOES ADICIONAIS]           " << endl;
    cout << "========================================" << endl;

    // 1. Estatisticas
    EstatisticasTelemetria est = calcularEstatisticasCompletas(dados);

    cout << "\n1) Estatisticas completas:" << endl;
    cout << "Velocidade media:          " << est.mediaVelocidade << endl;
    cout << "Desvio padrao velocidade:  " << est.desvioVelocidade << endl;
    cout << "Menor velocidade:          " << est.menorVelocidade << endl;
    cout << "Maior velocidade:          " << est.maiorVelocidade << endl;

    cout << "Elevacao media:            " << est.mediaElevacao << endl;
    cout << "Desvio padrao elevacao:    " << est.desvioElevacao << endl;
    cout << "Menor elevacao:            " << est.menorElevacao << endl;
    cout << "Maior elevacao:            " << est.maiorElevacao << endl;

    // 2. Deteccao e filtragem de anomalias
    float limiteVelocidade = static_cast<float>(est.mediaVelocidade + 2.0 * est.desvioVelocidade);
    float limiteSaltoElevacao = 80.0f;

    vector<AnomaliaGPS> anomalias = detectarAnomalias(
        dados,
        limiteVelocidade,
        limiteSaltoElevacao
    );

    vector<RegistroGPS> filtrados = filtrarRegistrosPorVelocidade(
        dados,
        limiteVelocidade
    );

    cout << "\n2) Deteccao e filtragem de anomalias:" << endl;
    cout << "Limite calculado de velocidade: " << limiteVelocidade << endl;
    cout << "Limite de salto de elevacao:    " << limiteSaltoElevacao << endl;
    cout << "Total de anomalias detectadas:  " << anomalias.size() << endl;
    cout << "Registros apos filtragem:       " << filtrados.size() << endl;

    if (!anomalias.empty()) {
        cout << "\nExemplos de anomalias detectadas:" << endl;

        int limiteExibicao = min(5, static_cast<int>(anomalias.size()));

        for (int i = 0; i < limiteExibicao; i++) {
            cout << "Indice: " << anomalias[i].indice
                 << " | Timestamp: " << anomalias[i].timestamp
                 << " | Motivo: " << anomalias[i].motivo
                 << " | Velocidade: " << anomalias[i].velocidade
                 << " | Elevacao: " << anomalias[i].elevacao
                 << endl;
        }
    }

    // 3. Classificacao de movimento
    ClassificacaoMovimento c = contarClassificacoes(dados);

    cout << "\n3) Classificacao de movimento:" << endl;
    cout << "Parado:                      " << c.parado << endl;
    cout << "Lento:                       " << c.lento << endl;
    cout << "Moderado:                    " << c.moderado << endl;
    cout << "Rapido:                      " << c.rapido << endl;

    cout << "========================================\n" << endl;
}

// ===============================
// BENCHMARKS COMPARATIVOS
// Mede insercao, busca, remocao, memoria e colisoes
// ===============================

struct ResultadoBenchmark {
    string estrutura;
    string operacao;
    int tamanhoEntrada;
    double tempoMs;
    double memoriaKB;
    long long colisoes;
};

double tempoAtualMs() {
    auto agora = chrono::high_resolution_clock::now();
    auto duracao = agora.time_since_epoch();
    return chrono::duration<double, milli>(duracao).count();
}

void salvarBenchmarksCSV(const vector<ResultadoBenchmark>& resultados, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);

    if (!arquivo.is_open()) {
        cerr << "Erro ao criar arquivo de benchmark: " << nomeArquivo << endl;
        return;
    }

    arquivo << "Estrutura,Operacao,TamanhoEntrada,TempoMs,MemoriaKB,Colisoes\n";

    for (const auto& r : resultados) {
        arquivo << r.estrutura << ","
                << r.operacao << ","
                << r.tamanhoEntrada << ","
                << fixed << setprecision(6) << r.tempoMs << ","
                << fixed << setprecision(3) << r.memoriaKB << ","
                << r.colisoes << "\n";
    }

    arquivo.close();

    cout << "\nArquivo de benchmark gerado: " << nomeArquivo << endl;
}

vector<int> gerarTamanhosTeste(int tamanhoTotal) {
    vector<int> tamanhos;

    if (tamanhoTotal >= 1000) tamanhos.push_back(1000);
    if (tamanhoTotal >= 5000) tamanhos.push_back(5000);
    if (tamanhoTotal >= 10000) tamanhos.push_back(10000);

    if (tamanhoTotal > 10000) {
        tamanhos.push_back(tamanhoTotal);
    }

    return tamanhos;
}

void benchmarkLista(
    const vector<RegistroGPS>& dados,
    int n,
    vector<ResultadoBenchmark>& resultados
) {
    ListaEncadeada lista;

    double inicio = tempoAtualMs();

    for (int i = 0; i < n; i++) {
        lista.inserirFim(dados[i].timestamp, i);
    }

    double fim = tempoAtualMs();

    resultados.push_back({
        "Lista Encadeada",
        "Insercao",
        n,
        fim - inicio,
        lista.memoriaEstimadaKB(),
        0
    });

    uint64_t alvo = dados[n / 2].timestamp;

    inicio = tempoAtualMs();

    int resultadoBusca = lista.buscar(alvo);
    (void)resultadoBusca;

    fim = tempoAtualMs();

    resultados.push_back({
        "Lista Encadeada",
        "Busca",
        n,
        fim - inicio,
        lista.memoriaEstimadaKB(),
        0
    });

    inicio = tempoAtualMs();

    lista.remover(alvo);

    fim = tempoAtualMs();

    resultados.push_back({
        "Lista Encadeada",
        "Remocao",
        n,
        fim - inicio,
        lista.memoriaEstimadaKB(),
        0
    });
}

void benchmarkAVL(
    const vector<RegistroGPS>& dados,
    int n,
    vector<ResultadoBenchmark>& resultados
) {
    ArvoreAVL avl;

    double inicio = tempoAtualMs();

    for (int i = 0; i < n; i++) {
        avl.inserir(dados[i].timestamp, i);
    }

    double fim = tempoAtualMs();

    resultados.push_back({
        "AVL",
        "Insercao",
        n,
        fim - inicio,
        avl.memoriaEstimadaKB(),
        0
    });

    uint64_t alvo = dados[n / 2].timestamp;

    inicio = tempoAtualMs();

    int resultadoBusca = avl.buscar(alvo);
    (void)resultadoBusca;

    fim = tempoAtualMs();

    resultados.push_back({
        "AVL",
        "Busca",
        n,
        fim - inicio,
        avl.memoriaEstimadaKB(),
        0
    });

    inicio = tempoAtualMs();

    avl.remover(alvo);

    fim = tempoAtualMs();

    resultados.push_back({
        "AVL",
        "Remocao",
        n,
        fim - inicio,
        avl.memoriaEstimadaKB(),
        0
    });
}

void benchmarkHash(
    const vector<RegistroGPS>& dados,
    int n,
    vector<ResultadoBenchmark>& resultados
) {
    TabelaHash hash(n * 2 + 1);

    double inicio = tempoAtualMs();

    for (int i = 0; i < n; i++) {
        hash.inserir(dados[i].timestamp, i);
    }

    double fim = tempoAtualMs();

    resultados.push_back({
        "Tabela Hash",
        "Insercao",
        n,
        fim - inicio,
        hash.memoriaEstimadaKB(),
        hash.colisoes()
    });

    uint64_t alvo = dados[n / 2].timestamp;

    inicio = tempoAtualMs();

    int resultadoBusca = hash.buscar(alvo);
    (void)resultadoBusca;

    fim = tempoAtualMs();

    resultados.push_back({
        "Tabela Hash",
        "Busca",
        n,
        fim - inicio,
        hash.memoriaEstimadaKB(),
        hash.colisoes()
    });

    inicio = tempoAtualMs();

    hash.remover(alvo);

    fim = tempoAtualMs();

    resultados.push_back({
        "Tabela Hash",
        "Remocao",
        n,
        fim - inicio,
        hash.memoriaEstimadaKB(),
        hash.colisoes()
    });
}

void benchmarkFenwick(
    const vector<RegistroGPS>& dados,
    int n,
    vector<ResultadoBenchmark>& resultados
) {
    FenwickTree fenwick(n);

    double inicio = tempoAtualMs();

    for (int i = 0; i < n; i++) {
        double ganhoElevacao = 0.0;

        if (i > 0) {
            ganhoElevacao = dados[i].elevacao - dados[i - 1].elevacao;

            if (ganhoElevacao < 0.0) {
                ganhoElevacao = 0.0;
            }
        }

        fenwick.atualizar(i, ganhoElevacao);
    }

    double fim = tempoAtualMs();

    resultados.push_back({
        "Fenwick Tree",
        "Construcao/Atualizacao",
        n,
        fim - inicio,
        fenwick.memoriaEstimadaKB(),
        0
    });

    int inicioIntervalo = n / 4;
    int fimIntervalo = n / 2;

    inicio = tempoAtualMs();

    double soma = fenwick.somaIntervalo(inicioIntervalo, fimIntervalo);
    (void)soma;

    fim = tempoAtualMs();

    resultados.push_back({
        "Fenwick Tree",
        "Consulta Intervalo",
        n,
        fim - inicio,
        fenwick.memoriaEstimadaKB(),
        0
    });

    int alvo = n / 3;

    inicio = tempoAtualMs();

    fenwick.remover(alvo);

    fim = tempoAtualMs();

    resultados.push_back({
        "Fenwick Tree",
        "Remocao Logica",
        n,
        fim - inicio,
        fenwick.memoriaEstimadaKB(),
        0
    });
}

void benchmarkKDTree(
    const vector<RegistroGPS>& dados,
    int n,
    vector<ResultadoBenchmark>& resultados
) {
    KDTree kdTree;

    double inicio = tempoAtualMs();

    for (int i = 0; i < n; i++) {
        kdTree.inserir(
            dados[i].latitude,
            dados[i].longitude,
            dados[i].timestamp,
            i
        );
    }

    double fim = tempoAtualMs();

    resultados.push_back({
        "KD-Tree",
        "Insercao",
        n,
        fim - inicio,
        kdTree.memoriaEstimadaKB(),
        0
    });

    int alvo = n / 2;

    inicio = tempoAtualMs();

    int indiceMaisProximo = kdTree.buscarMaisProximo(
        dados[alvo].latitude,
        dados[alvo].longitude
    );
    (void)indiceMaisProximo;

    fim = tempoAtualMs();

    resultados.push_back({
        "KD-Tree",
        "Busca Espacial",
        n,
        fim - inicio,
        kdTree.memoriaEstimadaKB(),
        0
    });

    inicio = tempoAtualMs();

    kdTree.removerPorTimestamp(dados[alvo].timestamp);

    fim = tempoAtualMs();

    resultados.push_back({
        "KD-Tree",
        "Remocao Logica",
        n,
        fim - inicio,
        kdTree.memoriaEstimadaKB(),
        0
    });
}

void executarBenchmarks(const vector<RegistroGPS>& dados) {
    cout << "\n========================================" << endl;
    cout << "          [BENCHMARKS DO SISTEMA]       " << endl;
    cout << "========================================" << endl;

    vector<ResultadoBenchmark> resultados;
    vector<int> tamanhos = gerarTamanhosTeste(static_cast<int>(dados.size()));

    for (int n : tamanhos) {
        cout << "\nExecutando benchmarks com " << n << " registros..." << endl;

        benchmarkLista(dados, n, resultados);
        benchmarkAVL(dados, n, resultados);
        benchmarkHash(dados, n, resultados);
        benchmarkFenwick(dados, n, resultados);
        benchmarkKDTree(dados, n, resultados);
    }

    cout << "\nResultados resumidos:" << endl;
    cout << left
         << setw(20) << "Estrutura"
         << setw(24) << "Operacao"
         << setw(10) << "N"
         << setw(14) << "Tempo(ms)"
         << setw(14) << "Memoria(KB)"
         << setw(10) << "Colisoes"
         << endl;

    cout << string(92, '-') << endl;

    for (const auto& r : resultados) {
        cout << left
             << setw(20) << r.estrutura
             << setw(24) << r.operacao
             << setw(10) << r.tamanhoEntrada
             << setw(14) << fixed << setprecision(6) << r.tempoMs
             << setw(14) << fixed << setprecision(3) << r.memoriaKB
             << setw(10) << r.colisoes
             << endl;
    }

    salvarBenchmarksCSV(resultados, "results/resultados_benchmark.csv");

    cout << "\nObservacao metodologica:" << endl;
    cout << "Lista, AVL e Hash sao comparadas em busca por timestamp." << endl;
    cout << "Fenwick Tree e avaliada separadamente em consultas de soma acumulada." << endl;
    cout << "KD-Tree e avaliada separadamente em busca espacial por latitude/longitude." << endl;
    cout << "Isso evita comparacoes invalidas entre estruturas com finalidades diferentes." << endl;

    cout << "========================================\n" << endl;
}
// ===============================
// TESTES COM RESTRICOES
// Exigencia do projeto:
// pelo menos uma restricao de cada categoria
// 1. Memoria
// 2. Processamento
// 3. Latencia
// 4. Dados
// 5. Algoritmica / Estrutural
// ===============================

struct ResultadoRestricao {
    string categoria;
    string restricao;
    string descricao;
    int entrada;
    int saida;
    double tempoMs;
    string observacao;
};

void salvarRestricoesCSV(const vector<ResultadoRestricao>& resultados, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);

    if (!arquivo.is_open()) {
        cerr << "Erro ao criar arquivo de restricoes: " << nomeArquivo << endl;
        return;
    }

    arquivo << "Categoria,Restricao,Descricao,Entrada,Saida,TempoMs,Observacao\n";

    for (const auto& r : resultados) {
        arquivo << r.categoria << ","
                << r.restricao << ","
                << r.descricao << ","
                << r.entrada << ","
                << r.saida << ","
                << fixed << setprecision(6) << r.tempoMs << ","
                << r.observacao << "\n";
    }

    arquivo.close();

    cout << "\nArquivo de restricoes gerado: " << nomeArquivo << endl;
}

// ===============================
// R5 - RESTRICAO DE MEMORIA
// Descarte automatico de dados antigos
// ===============================

void testeRestricaoMemoriaR5(
    const vector<RegistroGPS>& dados,
    vector<ResultadoRestricao>& resultados
) {
    double inicio = tempoAtualMs();

    int limiteRegistros = 10000;
    vector<RegistroGPS> bufferLimitado;

    if (static_cast<int>(dados.size()) > limiteRegistros) {
        bufferLimitado.assign(dados.end() - limiteRegistros, dados.end());
    } else {
        bufferLimitado = dados;
    }

    double fim = tempoAtualMs();

    int descartados = static_cast<int>(dados.size()) - static_cast<int>(bufferLimitado.size());

    resultados.push_back({
        "Memoria",
        "R5",
        "Descarte automatico de dados antigos",
        static_cast<int>(dados.size()),
        static_cast<int>(bufferLimitado.size()),
        fim - inicio,
        "Registros descartados: " + to_string(descartados)
    });
}

// ===============================
// R7 - RESTRICAO DE PROCESSAMENTO
// Cada busca deve terminar dentro de um limite de tempo
// ===============================

void testeRestricaoProcessamentoR7(
    const vector<RegistroGPS>& dados,
    vector<ResultadoRestricao>& resultados
) {
    int n = min(static_cast<int>(dados.size()), 10000);
    int consultas = min(n, 1000);

    TabelaHash hash(n * 2 + 1);

    for (int i = 0; i < n; i++) {
        hash.inserir(dados[i].timestamp, i);
    }

    double limiteMsPorOperacao = 0.05;
    int operacoesDentroDoLimite = 0;
    int operacoesForaDoLimite = 0;

    double inicioTotal = tempoAtualMs();

    for (int i = 0; i < consultas; i++) {
        uint64_t alvo = dados[(i * 37) % n].timestamp;

        double inicio = tempoAtualMs();

        int resultado = hash.buscar(alvo);
        (void)resultado;

        double fim = tempoAtualMs();

        double tempoOperacao = fim - inicio;

        if (tempoOperacao <= limiteMsPorOperacao) {
            operacoesDentroDoLimite++;
        } else {
            operacoesForaDoLimite++;
        }
    }

    double fimTotal = tempoAtualMs();

    resultados.push_back({
        "Processamento",
        "R7",
        "Orcamento maximo de tempo por operacao",
        consultas,
        operacoesDentroDoLimite,
        fimTotal - inicioTotal,
        "Fora do limite: " + to_string(operacoesForaDoLimite)
    });
}

// ===============================
// R13 - RESTRICAO DE LATENCIA
// Processamento em lote com atraso artificial
// ===============================

void testeRestricaoLatenciaR13(
    const vector<RegistroGPS>& dados,
    vector<ResultadoRestricao>& resultados
) {
    int n = min(static_cast<int>(dados.size()), 15000);
    int tamanhoLote = 5000;
    int atrasoMsPorLote = 20;
    int lotesProcessados = 0;

    double somaVelocidades = 0.0;

    double inicio = tempoAtualMs();

    for (int i = 0; i < n; i++) {
        somaVelocidades += dados[i].velocidade;

        if ((i + 1) % tamanhoLote == 0) {
            this_thread::sleep_for(chrono::milliseconds(atrasoMsPorLote));
            lotesProcessados++;
        }
    }

    double fim = tempoAtualMs();

    (void)somaVelocidades;

    resultados.push_back({
        "Latencia",
        "R13",
        "Processamento em lote com atraso artificial",
        n,
        lotesProcessados,
        fim - inicio,
        "Atraso por lote em ms: " + to_string(atrasoMsPorLote)
    });
}

// ===============================
// R16 - RESTRICAO DE DADOS
// Perda de 20 por cento das leituras
// ===============================

void testeRestricaoDadosR16(
    const vector<RegistroGPS>& dados,
    vector<ResultadoRestricao>& resultados
) {
    double inicio = tempoAtualMs();

    vector<RegistroGPS> dadosComPerda;
    dadosComPerda.reserve(dados.size());

    for (int i = 0; i < static_cast<int>(dados.size()); i++) {
        // Remove 1 a cada 5 registros, simulando perda de 20%
        if (i % 5 != 0) {
            dadosComPerda.push_back(dados[i]);
        }
    }

    double fim = tempoAtualMs();

    int perdidos = static_cast<int>(dados.size()) - static_cast<int>(dadosComPerda.size());

    resultados.push_back({
        "Dados",
        "R16",
        "Perda de 20 por cento das leituras",
        static_cast<int>(dados.size()),
        static_cast<int>(dadosComPerda.size()),
        fim - inicio,
        "Registros perdidos: " + to_string(perdidos)
    });
}

// ===============================
// R21 - RESTRICAO ALGORITMICA / ESTRUTURAL
// Substituir estrutura eficiente por uma menos eficiente
// Hash versus Lista Encadeada em busca por timestamp
// ===============================

void testeRestricaoAlgoritmicaR21(
    const vector<RegistroGPS>& dados,
    vector<ResultadoRestricao>& resultados
) {
    int n = min(static_cast<int>(dados.size()), 10000);
    int consultas = min(n, 1000);

    TabelaHash hash(n * 2 + 1);
    ListaEncadeada lista;

    for (int i = 0; i < n; i++) {
        hash.inserir(dados[i].timestamp, i);
        lista.inserirFim(dados[i].timestamp, i);
    }

    double inicioHash = tempoAtualMs();

    for (int i = 0; i < consultas; i++) {
        uint64_t alvo = dados[(i * 37) % n].timestamp;
        int resultado = hash.buscar(alvo);
        (void)resultado;
    }

    double fimHash = tempoAtualMs();

    double tempoHash = fimHash - inicioHash;

    double inicioLista = tempoAtualMs();

    for (int i = 0; i < consultas; i++) {
        uint64_t alvo = dados[(i * 37) % n].timestamp;
        int resultado = lista.buscar(alvo);
        (void)resultado;
    }

    double fimLista = tempoAtualMs();

    double tempoLista = fimLista - inicioLista;

    resultados.push_back({
        "Algoritmica",
        "R21 Hash",
        "Estrutura eficiente para busca por timestamp",
        consultas,
        consultas,
        tempoHash,
        "Busca usando tabela hash"
    });

    resultados.push_back({
        "Algoritmica",
        "R21 Lista",
        "Substituicao por estrutura menos eficiente",
        consultas,
        consultas,
        tempoLista,
        "Busca usando lista encadeada"
    });
}

// ===============================
// EXECUCAO GERAL DOS TESTES
// ===============================

void executarTestesRestricoes(const vector<RegistroGPS>& dados) {
    cout << "\n========================================" << endl;
    cout << "       [TESTES COM RESTRICOES]          " << endl;
    cout << "========================================" << endl;

    vector<ResultadoRestricao> resultados;

    testeRestricaoMemoriaR5(dados, resultados);
    testeRestricaoProcessamentoR7(dados, resultados);
    testeRestricaoLatenciaR13(dados, resultados);
    testeRestricaoDadosR16(dados, resultados);
    testeRestricaoAlgoritmicaR21(dados, resultados);

    cout << left
         << setw(16) << "Categoria"
         << setw(14) << "Restricao"
         << setw(46) << "Descricao"
         << setw(12) << "Entrada"
         << setw(12) << "Saida"
         << setw(14) << "Tempo(ms)"
         << "Observacao"
         << endl;

    cout << string(130, '-') << endl;

    for (const auto& r : resultados) {
        cout << left
             << setw(16) << r.categoria
             << setw(14) << r.restricao
             << setw(46) << r.descricao
             << setw(12) << r.entrada
             << setw(12) << r.saida
             << setw(14) << fixed << setprecision(6) << r.tempoMs
             << r.observacao
             << endl;
    }

    salvarRestricoesCSV(resultados, "results/resultados_restricoes.csv");

    cout << "\nRestricoes implementadas:" << endl;
    cout << "R5  - Memoria: descarte automatico de dados antigos." << endl;
    cout << "R7  - Processamento: limite maximo de tempo por operacao." << endl;
    cout << "R13 - Latencia: processamento em lote com atraso artificial." << endl;
    cout << "R16 - Dados: perda de 20 por cento das leituras." << endl;
    cout << "R21 - Algoritmica: troca de Hash por Lista Encadeada." << endl;

    cout << "========================================\n" << endl;
}
// ===============================
// TABELA HASH OTIMIZADA
// Versao otimizada da TabelaHash tradicional
// Usa enderecamento aberto com sondagem linear
// Objetivo: melhorar localidade de memoria e reduzir uso de ponteiros
// ===============================

class TabelaHashOtimizada {
private:
    vector<uint64_t> chaves;
    vector<int> indices;
    vector<char> estado; 
    // estado:
    // 0 = vazio
    // 1 = ocupado
    // 2 = removido

    int capacidade;
    int totalElementos;
    long long totalColisoes;

    static uint64_t misturarHash(uint64_t x) {
        // Funcao de mistura baseada em SplitMix64
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        return x;
    }

    int proximaPotenciaDeDois(int valor) {
        int pot = 1;

        while (pot < valor) {
            pot *= 2;
        }

        return pot;
    }

    int funcaoHash(uint64_t timestamp) const {
        return static_cast<int>(misturarHash(timestamp) & (capacidade - 1));
    }

public:
    TabelaHashOtimizada(int capacidadeMinima) {
        capacidade = proximaPotenciaDeDois(max(16, capacidadeMinima));
        chaves.assign(capacidade, 0);
        indices.assign(capacidade, -1);
        estado.assign(capacidade, 0);

        totalElementos = 0;
        totalColisoes = 0;
    }

    bool inserir(uint64_t timestamp, int indice) {
        if (totalElementos >= capacidade * 0.7) {
            return false;
        }

        int pos = funcaoHash(timestamp);
        int primeiraRemovida = -1;

        for (int tentativa = 0; tentativa < capacidade; tentativa++) {
            int atual = (pos + tentativa) & (capacidade - 1);

            if (estado[atual] == 1) {
                if (chaves[atual] == timestamp) {
                    indices[atual] = indice;
                    return false;
                }

                totalColisoes++;
            } 
            else if (estado[atual] == 2) {
                if (primeiraRemovida == -1) {
                    primeiraRemovida = atual;
                }
            } 
            else {
                int destino = primeiraRemovida != -1 ? primeiraRemovida : atual;

                chaves[destino] = timestamp;
                indices[destino] = indice;
                estado[destino] = 1;
                totalElementos++;

                return true;
            }
        }

        if (primeiraRemovida != -1) {
            chaves[primeiraRemovida] = timestamp;
            indices[primeiraRemovida] = indice;
            estado[primeiraRemovida] = 1;
            totalElementos++;

            return true;
        }

        return false;
    }

    int buscar(uint64_t timestamp) const {
        int pos = funcaoHash(timestamp);

        for (int tentativa = 0; tentativa < capacidade; tentativa++) {
            int atual = (pos + tentativa) & (capacidade - 1);

            if (estado[atual] == 0) {
                return -1;
            }

            if (estado[atual] == 1 && chaves[atual] == timestamp) {
                return indices[atual];
            }
        }

        return -1;
    }

    bool remover(uint64_t timestamp) {
        int pos = funcaoHash(timestamp);

        for (int tentativa = 0; tentativa < capacidade; tentativa++) {
            int atual = (pos + tentativa) & (capacidade - 1);

            if (estado[atual] == 0) {
                return false;
            }

            if (estado[atual] == 1 && chaves[atual] == timestamp) {
                estado[atual] = 2;
                indices[atual] = -1;
                totalElementos--;

                return true;
            }
        }

        return false;
    }

    int tamanho() const {
        return totalElementos;
    }

    int getCapacidade() const {
        return capacidade;
    }

    long long colisoes() const {
        return totalColisoes;
    }

    double fatorCarga() const {
        return static_cast<double>(totalElementos) / capacidade;
    }

    double memoriaEstimadaKB() const {
        double memoriaChaves = chaves.size() * sizeof(uint64_t);
        double memoriaIndices = indices.size() * sizeof(int);
        double memoriaEstado = estado.size() * sizeof(char);

        return (memoriaChaves + memoriaIndices + memoriaEstado) / 1024.0;
    }
};
// ===============================
// COMPARACAO ENTRE HASH ORIGINAL E HASH OTIMIZADA
// Exigencia: estrutura otimizada comparada com a original
// ===============================

struct ResultadoOtimizacao {
    string estrutura;
    string operacao;
    int tamanhoEntrada;
    double tempoMs;
    double memoriaKB;
    long long colisoes;
};

void salvarComparacaoHashCSV(
    const vector<ResultadoOtimizacao>& resultados,
    const string& nomeArquivo
) {
    ofstream arquivo(nomeArquivo);

    if (!arquivo.is_open()) {
        cerr << "Erro ao criar arquivo de comparacao: " << nomeArquivo << endl;
        return;
    }

    arquivo << "Estrutura,Operacao,TamanhoEntrada,TempoMs,MemoriaKB,Colisoes\n";

    for (const auto& r : resultados) {
        arquivo << r.estrutura << ","
                << r.operacao << ","
                << r.tamanhoEntrada << ","
                << fixed << setprecision(6) << r.tempoMs << ","
                << fixed << setprecision(3) << r.memoriaKB << ","
                << r.colisoes << "\n";
    }

    arquivo.close();

    cout << "\nArquivo de comparacao gerado: " << nomeArquivo << endl;
}

void executarComparacaoHashOtimizada(const vector<RegistroGPS>& dados) {
    cout << "\n========================================" << endl;
    cout << "   [COMPARACAO HASH ORIGINAL X OTIMIZADA]" << endl;
    cout << "========================================" << endl;

    int n = static_cast<int>(dados.size());
    int consultas = min(n, 2000);

    vector<ResultadoOtimizacao> resultados;

    // ===============================
    // HASH ORIGINAL - INSERCAO
    // ===============================

    TabelaHash hashOriginal(n * 2 + 1);

    double inicio = tempoAtualMs();

    for (int i = 0; i < n; i++) {
        hashOriginal.inserir(dados[i].timestamp, i);
    }

    double fim = tempoAtualMs();

    resultados.push_back({
        "Hash Original",
        "Insercao",
        n,
        fim - inicio,
        hashOriginal.memoriaEstimadaKB(),
        hashOriginal.colisoes()
    });

    // ===============================
    // HASH OTIMIZADA - INSERCAO
    // ===============================

    TabelaHashOtimizada hashOtimizada(n * 2 + 1);

    inicio = tempoAtualMs();

    for (int i = 0; i < n; i++) {
        hashOtimizada.inserir(dados[i].timestamp, i);
    }

    fim = tempoAtualMs();

    resultados.push_back({
        "Hash Otimizada",
        "Insercao",
        n,
        fim - inicio,
        hashOtimizada.memoriaEstimadaKB(),
        hashOtimizada.colisoes()
    });

    // ===============================
    // BUSCAS
    // ===============================

    inicio = tempoAtualMs();

    for (int i = 0; i < consultas; i++) {
        uint64_t alvo = dados[(i * 37) % n].timestamp;
        int resultado = hashOriginal.buscar(alvo);
        (void)resultado;
    }

    fim = tempoAtualMs();

    resultados.push_back({
        "Hash Original",
        "Busca",
        consultas,
        fim - inicio,
        hashOriginal.memoriaEstimadaKB(),
        hashOriginal.colisoes()
    });

    inicio = tempoAtualMs();

    for (int i = 0; i < consultas; i++) {
        uint64_t alvo = dados[(i * 37) % n].timestamp;
        int resultado = hashOtimizada.buscar(alvo);
        (void)resultado;
    }

    fim = tempoAtualMs();

    resultados.push_back({
        "Hash Otimizada",
        "Busca",
        consultas,
        fim - inicio,
        hashOtimizada.memoriaEstimadaKB(),
        hashOtimizada.colisoes()
    });

    // ===============================
    // REMOCOES
    // ===============================

    inicio = tempoAtualMs();

    for (int i = 0; i < consultas; i++) {
        uint64_t alvo = dados[(i * 37) % n].timestamp;
        hashOriginal.remover(alvo);
    }

    fim = tempoAtualMs();

    resultados.push_back({
        "Hash Original",
        "Remocao",
        consultas,
        fim - inicio,
        hashOriginal.memoriaEstimadaKB(),
        hashOriginal.colisoes()
    });

    inicio = tempoAtualMs();

    for (int i = 0; i < consultas; i++) {
        uint64_t alvo = dados[(i * 37) % n].timestamp;
        hashOtimizada.remover(alvo);
    }

    fim = tempoAtualMs();

    resultados.push_back({
        "Hash Otimizada",
        "Remocao",
        consultas,
        fim - inicio,
        hashOtimizada.memoriaEstimadaKB(),
        hashOtimizada.colisoes()
    });

    // ===============================
    // EXIBICAO
    // ===============================

    cout << left
         << setw(18) << "Estrutura"
         << setw(14) << "Operacao"
         << setw(12) << "Entrada"
         << setw(14) << "Tempo(ms)"
         << setw(14) << "Memoria(KB)"
         << setw(10) << "Colisoes"
         << endl;

    cout << string(82, '-') << endl;

    for (const auto& r : resultados) {
        cout << left
             << setw(18) << r.estrutura
             << setw(14) << r.operacao
             << setw(12) << r.tamanhoEntrada
             << setw(14) << fixed << setprecision(6) << r.tempoMs
             << setw(14) << fixed << setprecision(3) << r.memoriaKB
             << setw(10) << r.colisoes
             << endl;
    }

    salvarComparacaoHashCSV(resultados, "results/comparacao_hash_otimizada.csv");
    
    cout << "\nJustificativa da otimizacao:" << endl;
    cout << "A Hash Original usa encadeamento separado com alocacao dinamica de nos." << endl;
    cout << "A Hash Otimizada usa vetores contiguos e enderecamento aberto." << endl;
    cout << "Isso melhora a localidade de memoria e reduz o custo de ponteiros." << endl;
    cout << "A comparacao avalia insercao, busca, remocao, memoria e colisoes." << endl;

    cout << "========================================\n" << endl;
}
// ===============================
// MENU FINAL / VISUALIZACAO INTERATIVA
// Permite explorar o sistema pelo terminal
// ===============================

void exibirPrimeirosRegistros(const vector<RegistroGPS>& dados, int quantidade) {
    cout << "\nPrimeiros " << quantidade << " registros:" << endl;
    cout << left
         << setw(8) << "Indice"
         << setw(18) << "Timestamp"
         << setw(14) << "Latitude"
         << setw(14) << "Longitude"
         << setw(12) << "Elevacao"
         << setw(12) << "Velocidade"
         << endl;

    cout << string(78, '-') << endl;

    int limite = min(quantidade, static_cast<int>(dados.size()));

    for (int i = 0; i < limite; i++) {
        cout << left
             << setw(8) << i
             << setw(18) << dados[i].timestamp
             << setw(14) << dados[i].latitude
             << setw(14) << dados[i].longitude
             << setw(12) << dados[i].elevacao
             << setw(12) << dados[i].velocidade
             << endl;
    }
}

void consultaPorTimestampMenu(const vector<RegistroGPS>& dados) {
    TabelaHash hash(static_cast<int>(dados.size() * 2 + 1));

    for (int i = 0; i < static_cast<int>(dados.size()); i++) {
        hash.inserir(dados[i].timestamp, i);
    }

    uint64_t timestamp;
    cout << "\nDigite o timestamp que deseja buscar: ";
    cin >> timestamp;

    int indice = hash.buscar(timestamp);

    if (indice == -1) {
        cout << "Registro nao encontrado." << endl;
        return;
    }

    cout << "\nRegistro encontrado:" << endl;
    cout << "Indice:      " << indice << endl;
    cout << "Timestamp:   " << dados[indice].timestamp << endl;
    cout << "Latitude:    " << dados[indice].latitude << endl;
    cout << "Longitude:   " << dados[indice].longitude << endl;
    cout << "Elevacao:    " << dados[indice].elevacao << endl;
    cout << "Velocidade:  " << dados[indice].velocidade << endl;
}

void consultaEspacialMenu(const vector<RegistroGPS>& dados) {
    KDTree kdTree;

    for (int i = 0; i < static_cast<int>(dados.size()); i++) {
        kdTree.inserir(
            dados[i].latitude,
            dados[i].longitude,
            dados[i].timestamp,
            i
        );
    }

    double latitude;
    double longitude;

    cout << "\nDigite a latitude: ";
    cin >> latitude;

    cout << "Digite a longitude: ";
    cin >> longitude;

    int indice = kdTree.buscarMaisProximo(latitude, longitude);

    if (indice == -1) {
        cout << "Nenhum ponto encontrado." << endl;
        return;
    }

    cout << "\nPonto mais proximo encontrado pela KD-Tree:" << endl;
    cout << "Indice:      " << indice << endl;
    cout << "Timestamp:   " << dados[indice].timestamp << endl;
    cout << "Latitude:    " << dados[indice].latitude << endl;
    cout << "Longitude:   " << dados[indice].longitude << endl;
    cout << "Elevacao:    " << dados[indice].elevacao << endl;
    cout << "Velocidade:  " << dados[indice].velocidade << endl;
}

void consultaGanhoElevacaoMenu(const vector<RegistroGPS>& dados) {
    FenwickTree fenwick(static_cast<int>(dados.size()));

    for (int i = 0; i < static_cast<int>(dados.size()); i++) {
        double ganhoElevacao = 0.0;

        if (i > 0) {
            ganhoElevacao = dados[i].elevacao - dados[i - 1].elevacao;

            if (ganhoElevacao < 0.0) {
                ganhoElevacao = 0.0;
            }
        }

        fenwick.atualizar(i, ganhoElevacao);
    }

    int inicio;
    int fim;

    cout << "\nDigite o indice inicial: ";
    cin >> inicio;

    cout << "Digite o indice final: ";
    cin >> fim;

    double ganho = fenwick.somaIntervalo(inicio, fim);

    cout << "\nGanho positivo de elevacao no intervalo: " << ganho << endl;
}

void exibirArquivosGerados() {
    cout << "\nArquivos CSV gerados pelo sistema:" << endl;
    cout << "- results/resultados_benchmark.csv" << endl;
    cout << "- results/resultados_restricoes.csv" << endl;
    cout << "- results/comparacao_hash_otimizada.csv" << endl;

    cout << "\nEsses arquivos podem ser usados para montar tabelas e graficos no relatorio." << endl;
}

void executarMenuFinal(const vector<RegistroGPS>& dados) {
    string entrada;
    int opcao = -1;

    while (opcao != 0) {
        cout << "\n========================================" << endl;
        cout << "        MENU FINAL DO SISTEMA GPS        " << endl;
        cout << "========================================" << endl;
        cout << "1 - Exibir status do sistema" << endl;
        cout << "2 - Exibir primeiros registros" << endl;
        cout << "3 - Buscar registro por timestamp usando Hash" << endl;
        cout << "4 - Buscar ponto geografico mais proximo usando KD-Tree" << endl;
        cout << "5 - Consultar ganho de elevacao usando Fenwick Tree" << endl;
        cout << "6 - Exibir operacoes adicionais" << endl;
        cout << "7 - Exibir arquivos CSV gerados" << endl;
        cout << "0 - Sair" << endl;
        cout << "Escolha uma opcao: " << flush;

        getline(cin, entrada);

        if (entrada.empty()) {
            cout << "Nenhuma opcao digitada. Tente novamente." << endl;
            continue;
        }

        try {
            opcao = stoi(entrada);
        } catch (...) {
            cout << "Opcao invalida. Digite apenas um numero." << endl;
            continue;
        }

        switch (opcao) {
            case 1:
                exibirStatusSistema(dados);
                break;

            case 2:
                exibirPrimeirosRegistros(dados, 10);
                break;

            case 3:
                consultaPorTimestampMenu(dados);
                break;

            case 4:
                consultaEspacialMenu(dados);
                break;

            case 5:
                consultaGanhoElevacaoMenu(dados);
                break;

            case 6:
                exibirOperacoesAdicionais(dados);
                break;

            case 7:
                exibirArquivosGerados();
                break;

            case 0:
                cout << "Encerrando visualizacao interativa." << endl;
                break;

            default:
                cout << "Opcao invalida." << endl;
                break;
        }

        if (opcao != 0) {
            cout << "\nPressione ENTER para voltar ao menu..." << flush;
            getline(cin, entrada);
        }
    }
}

int main() {
    cout << "Iniciando sistema de telemetria GPS..." << endl;

    vector<RegistroGPS> buffer = carregarDados("data/dataset_telemetria.csv");

    if (buffer.empty()) {
        cerr << "Erro: nenhum dado valido foi carregado." << endl;
        return 1;
    }

    ordenarPorTimestamp(buffer);
    removerTimestampsDuplicados(buffer);

    if (buffer.size() < 10000) {
        cerr << "\nERRO: O dataset possui menos de 10.000 amostras validas." << endl;
        cerr << "Quantidade atual: " << buffer.size() << endl;
        cerr << "O PDF exige no minimo 10.000 amostras." << endl;
        return 1;
    }

    exibirStatusSistema(buffer);

    cout << "Parte 1 concluida com sucesso." << endl;

    // ===============================
    // TESTE DA ARVORE AVL
    // ===============================

    cout << "\nConstruindo indice AVL por timestamp..." << endl;

    ArvoreAVL avl;

    for (int i = 0; i < static_cast<int>(buffer.size()); i++) {
        avl.inserir(buffer[i].timestamp, i);
    }

    cout << "AVL construida com sucesso." << endl;
    cout << "Total de nos na AVL:        " << avl.tamanho() << endl;
    cout << "Altura da AVL:              " << avl.alturaArvore() << endl;
    cout << "Memoria estimada da AVL:    " << avl.memoriaEstimadaKB() << " KB" << endl;
    cout << "AVL balanceada:             " << (avl.estaBalanceada() ? "Sim" : "Nao") << endl;

    // Teste de busca
    uint64_t timestampTeste = buffer[buffer.size() / 2].timestamp;
    int indiceEncontrado = avl.buscar(timestampTeste);

    cout << "\nTeste de busca na AVL:" << endl;
    cout << "Timestamp buscado:          " << timestampTeste << endl;

    if (indiceEncontrado != -1) {
        cout << "Registro encontrado no indice: " << indiceEncontrado << endl;
        cout << "Latitude:                   " << buffer[indiceEncontrado].latitude << endl;
        cout << "Longitude:                  " << buffer[indiceEncontrado].longitude << endl;
        cout << "Elevacao:                   " << buffer[indiceEncontrado].elevacao << endl;
        cout << "Velocidade:                 " << buffer[indiceEncontrado].velocidade << endl;
    } else {
        cout << "Registro nao encontrado." << endl;
    }

    // Teste de remocao
    cout << "\nTeste de remocao na AVL:" << endl;

    bool removeu = avl.remover(timestampTeste);

    cout << "Remocao realizada:          " << (removeu ? "Sim" : "Nao") << endl;
    cout << "Busca apos remocao:         "
         << (avl.buscar(timestampTeste) == -1 ? "Nao encontrado" : "Ainda encontrado")
         << endl;

    cout << "Total de nos apos remocao:  " << avl.tamanho() << endl;
    cout << "Altura apos remocao:        " << avl.alturaArvore() << endl;
    cout << "AVL balanceada apos remocao:" << (avl.estaBalanceada() ? " Sim" : " Nao") << endl;

    cout << "\nParte 2 concluida com sucesso." << endl;

// ===============================
// TESTE DA LISTA ENCADEADA
// ===============================

cout << "\nConstruindo Lista Encadeada..." << endl;

ListaEncadeada lista;

for (int i = 0; i < static_cast<int>(buffer.size()); i++) {
    lista.inserirFim(buffer[i].timestamp, i);
}

cout << "Lista Encadeada construida com sucesso." << endl;
cout << "Total de nos na lista:      " << lista.tamanho() << endl;
cout << "Memoria estimada da lista:  " << lista.memoriaEstimadaKB() << " KB" << endl;

// Teste de busca na lista
uint64_t timestampLista = buffer[buffer.size() / 3].timestamp;
int indiceLista = lista.buscar(timestampLista);

cout << "\nTeste de busca na Lista Encadeada:" << endl;
cout << "Timestamp buscado:          " << timestampLista << endl;

if (indiceLista != -1) {
    cout << "Registro encontrado no indice: " << indiceLista << endl;
    cout << "Latitude:                   " << buffer[indiceLista].latitude << endl;
    cout << "Longitude:                  " << buffer[indiceLista].longitude << endl;
    cout << "Elevacao:                   " << buffer[indiceLista].elevacao << endl;
    cout << "Velocidade:                 " << buffer[indiceLista].velocidade << endl;
} else {
    cout << "Registro nao encontrado." << endl;
}

// Teste de remocao na lista
cout << "\nTeste de remocao na Lista Encadeada:" << endl;

bool removeuLista = lista.remover(timestampLista);

cout << "Remocao realizada:          " << (removeuLista ? "Sim" : "Nao") << endl;
cout << "Busca apos remocao:         "
     << (lista.buscar(timestampLista) == -1 ? "Nao encontrado" : "Ainda encontrado")
     << endl;

cout << "Total de nos apos remocao:  " << lista.tamanho() << endl;

cout << "\nParte 3 concluida com sucesso." << endl;
cout << "\nParte 3 concluida com sucesso." << endl;

// ===============================
// TESTE DA TABELA HASH
// ===============================

cout << "\nConstruindo Tabela Hash..." << endl;

// Capacidade maior que o numero de registros para reduzir colisoes
TabelaHash hash(static_cast<int>(buffer.size() * 2 + 1));

for (int i = 0; i < static_cast<int>(buffer.size()); i++) {
    hash.inserir(buffer[i].timestamp, i);
}

cout << "Tabela Hash construida com sucesso." << endl;
cout << "Total de elementos na Hash: " << hash.tamanho() << endl;
cout << "Capacidade da Hash:         " << hash.getCapacidade() << endl;
cout << "Fator de carga:             " << hash.fatorCarga() << endl;
cout << "Total de colisoes:          " << hash.colisoes() << endl;
cout << "Memoria estimada da Hash:   " << hash.memoriaEstimadaKB() << " KB" << endl;

// Teste de busca na Hash
uint64_t timestampHash = buffer[buffer.size() / 4].timestamp;
int indiceHash = hash.buscar(timestampHash);

cout << "\nTeste de busca na Tabela Hash:" << endl;
cout << "Timestamp buscado:          " << timestampHash << endl;

if (indiceHash != -1) {
    cout << "Registro encontrado no indice: " << indiceHash << endl;
    cout << "Latitude:                   " << buffer[indiceHash].latitude << endl;
    cout << "Longitude:                  " << buffer[indiceHash].longitude << endl;
    cout << "Elevacao:                   " << buffer[indiceHash].elevacao << endl;
    cout << "Velocidade:                 " << buffer[indiceHash].velocidade << endl;
} else {
    cout << "Registro nao encontrado." << endl;
}

// Teste de remocao na Hash
cout << "\nTeste de remocao na Tabela Hash:" << endl;

bool removeuHash = hash.remover(timestampHash);

cout << "Remocao realizada:          " << (removeuHash ? "Sim" : "Nao") << endl;
cout << "Busca apos remocao:         "
     << (hash.buscar(timestampHash) == -1 ? "Nao encontrado" : "Ainda encontrado")
     << endl;

cout << "Total de elementos apos remocao: " << hash.tamanho() << endl;

cout << "\nParte 4 concluida com sucesso." << endl;
cout << "\nParte 4 concluida com sucesso." << endl;

// ===============================
// TESTE DA FENWICK TREE
// ===============================

cout << "\nConstruindo Fenwick Tree para ganho de elevacao..." << endl;

FenwickTree fenwick(static_cast<int>(buffer.size()));

// A Fenwick armazenara o ganho positivo de elevacao entre registros consecutivos
for (int i = 0; i < static_cast<int>(buffer.size()); i++) {
    double ganhoElevacao = 0.0;

    if (i > 0) {
        ganhoElevacao = buffer[i].elevacao - buffer[i - 1].elevacao;

        if (ganhoElevacao < 0.0) {
            ganhoElevacao = 0.0;
        }
    }

    fenwick.atualizar(i, ganhoElevacao);
}

cout << "Fenwick Tree construida com sucesso." << endl;
cout << "Total de posicoes:          " << fenwick.tamanho() << endl;
cout << "Memoria estimada Fenwick:   " << fenwick.memoriaEstimadaKB() << " KB" << endl;

// Teste de consulta de intervalo
int inicioIntervalo = static_cast<int>(buffer.size() / 4);
int fimIntervalo = static_cast<int>(buffer.size() / 2);

double ganhoIntervalo = fenwick.somaIntervalo(inicioIntervalo, fimIntervalo);

cout << "\nTeste de consulta na Fenwick Tree:" << endl;
cout << "Intervalo consultado:       " << inicioIntervalo << " ate " << fimIntervalo << endl;
cout << "Ganho de elevacao no trecho:" << ganhoIntervalo << endl;

// Teste de busca de valor individual
int indiceFenwick = static_cast<int>(buffer.size() / 3);
double valorIndividual = fenwick.buscarValor(indiceFenwick);

cout << "\nTeste de busca individual na Fenwick Tree:" << endl;
cout << "Indice buscado:             " << indiceFenwick << endl;
cout << "Ganho armazenado no indice: " << valorIndividual << endl;

// Teste de remocao logica
cout << "\nTeste de remocao na Fenwick Tree:" << endl;

bool removeuFenwick = fenwick.remover(indiceFenwick);

cout << "Remocao realizada:          " << (removeuFenwick ? "Sim" : "Nao") << endl;
cout << "Valor apos remocao:         " << fenwick.buscarValor(indiceFenwick) << endl;

double ganhoIntervaloAposRemocao = fenwick.somaIntervalo(inicioIntervalo, fimIntervalo);

cout << "Ganho do intervalo apos remocao: "
     << ganhoIntervaloAposRemocao << endl;

cout << "\nParte 5 concluida com sucesso." << endl;
cout << "\nParte 5 concluida com sucesso." << endl;

// ===============================
// TESTE DA KD-TREE
// ===============================

cout << "\nConstruindo KD-Tree para busca espacial..." << endl;

KDTree kdTree;

for (int i = 0; i < static_cast<int>(buffer.size()); i++) {
    kdTree.inserir(
        buffer[i].latitude,
        buffer[i].longitude,
        buffer[i].timestamp,
        i
    );
}

cout << "KD-Tree construida com sucesso." << endl;
cout << "Total de nos na KD-Tree:    " << kdTree.tamanhoTotal() << endl;
cout << "Nos ativos na KD-Tree:      " << kdTree.tamanhoAtivo() << endl;
cout << "Altura da KD-Tree:          " << kdTree.alturaArvore() << endl;
cout << "Memoria estimada KD-Tree:   " << kdTree.memoriaEstimadaKB() << " KB" << endl;

// Teste de busca espacial
int indiceAlvoKD = static_cast<int>(buffer.size() * 2 / 3);

double latitudeAlvo = buffer[indiceAlvoKD].latitude;
double longitudeAlvo = buffer[indiceAlvoKD].longitude;

int indiceMaisProximo = kdTree.buscarMaisProximo(latitudeAlvo, longitudeAlvo);

cout << "\nTeste de busca na KD-Tree:" << endl;
cout << "Latitude buscada:           " << latitudeAlvo << endl;
cout << "Longitude buscada:          " << longitudeAlvo << endl;

if (indiceMaisProximo != -1) {
    cout << "Indice mais proximo:        " << indiceMaisProximo << endl;
    cout << "Timestamp encontrado:       " << buffer[indiceMaisProximo].timestamp << endl;
    cout << "Latitude encontrada:        " << buffer[indiceMaisProximo].latitude << endl;
    cout << "Longitude encontrada:       " << buffer[indiceMaisProximo].longitude << endl;
    cout << "Velocidade:                 " << buffer[indiceMaisProximo].velocidade << endl;
} else {
    cout << "Nenhum ponto encontrado." << endl;
}

// Teste de remocao logica
cout << "\nTeste de remocao na KD-Tree:" << endl;

uint64_t timestampKD = buffer[indiceAlvoKD].timestamp;

bool removeuKD = kdTree.removerPorTimestamp(timestampKD);

cout << "Timestamp removido:         " << timestampKD << endl;
cout << "Remocao realizada:          " << (removeuKD ? "Sim" : "Nao") << endl;
cout << "Nos ativos apos remocao:    " << kdTree.tamanhoAtivo() << endl;

int novoMaisProximo = kdTree.buscarMaisProximo(latitudeAlvo, longitudeAlvo);

cout << "Busca apos remocao:         ";

if (novoMaisProximo != -1) {
    cout << "novo indice mais proximo = " << novoMaisProximo << endl;
    cout << "Novo timestamp encontrado:  " << buffer[novoMaisProximo].timestamp << endl;
} else {
    cout << "nenhum ponto encontrado" << endl;
}

cout << "\nParte 6 concluida com sucesso." << endl;
cout << "Todas as 5 estruturas obrigatorias foram implementadas." << endl;
cout << "\nParte 6 concluida com sucesso." << endl;
cout << "Todas as 5 estruturas obrigatorias foram implementadas." << endl;

// ===============================
// TESTE DAS OPERACOES ADICIONAIS
// ===============================

exibirOperacoesAdicionais(buffer);

cout << "\nParte 7 concluida com sucesso." << endl;
cout << "Operacoes adicionais implementadas:" << endl;
cout << "1. Estatisticas completas dos dados" << endl;
cout << "2. Deteccao e filtragem de anomalias" << endl;
cout << "3. Classificacao de movimento" << endl;

cout << "\nParte 7 concluida com sucesso." << endl;
cout << "Operacoes adicionais implementadas:" << endl;
cout << "1. Estatisticas completas dos dados" << endl;
cout << "2. Deteccao e filtragem de anomalias" << endl;
cout << "3. Classificacao de movimento" << endl;

// ===============================
// BENCHMARKS COMPARATIVOS
// ===============================

executarBenchmarks(buffer);

cout << "\nParte 8 concluida com sucesso." << endl;
cout << "Benchmarks implementados e arquivo results/resultados_benchmark.csv gerado." << endl;
cout << "\nParte 8 concluida com sucesso." << endl;
cout << "Benchmarks implementados e arquivo resultados_benchmark.csv gerado." << endl;

// ===============================
// TESTES COM RESTRICOES
// ===============================

executarTestesRestricoes(buffer);

cout << "\nParte 9 concluida com sucesso." << endl;
cout << "Cinco testes com restricoes implementados." << endl;
cout << "Arquivo results/resultados_restricoes.csv gerado." << endl;
cout << "\nParte 9 concluida com sucesso." << endl;
cout << "Cinco testes com restricoes implementados." << endl;
cout << "Arquivo results/resultados_restricoes.csv gerado." << endl;

// ===============================
// ESTRUTURA OTIMIZADA
// ===============================

executarComparacaoHashOtimizada(buffer);

cout << "\nParte 10 concluida com sucesso." << endl;
cout << "Estrutura otimizada implementada: Hash Otimizada." << endl;
cout << "Comparacao com a Hash Original realizada." << endl;
cout << "Arquivo results/comparacao_hash_otimizada.csv gerado." << endl;

cout << "\nParte 10 concluida com sucesso." << endl;
cout << "Estrutura otimizada implementada: Hash Otimizada." << endl;
cout << "Comparacao com a Hash Original realizada." << endl;
cout << "Arquivo results/comparacao_hash_otimizada.csv gerado." << endl;

// ===============================
// MENU FINAL / VISUALIZACAO
// ===============================

cout << "\nParte 11 iniciada: menu final de visualizacao." << endl;
cin.ignore(numeric_limits<streamsize>::max(), '\n');
executarMenuFinal(buffer);

cout << "\nSistema finalizado com sucesso." << endl;
cout << "Projeto contem:" << endl;
cout << "- 5 estruturas de dados" << endl;
cout << "- 3 operacoes adicionais" << endl;
cout << "- Benchmarks comparativos" << endl;
cout << "- 5 testes com restricoes" << endl;
cout << "- Hash otimizada comparada com Hash original" << endl;
cout << "- Visualizacao interativa por terminal" << endl;

return 0;
}

