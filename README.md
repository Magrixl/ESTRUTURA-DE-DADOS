# Sistema de Telemetria GPS com Estruturas de Dados

## 1. Descrição do Projeto

Este projeto foi desenvolvido para a disciplina de Estrutura de Dados, conforme a proposta do Projeto de Estrutura de Dados 2026.

O sistema simula uma aplicação de telemetria GPS, manipulando registros compostos por:

- timestamp;
- latitude;
- longitude;
- elevação;
- velocidade.

O objetivo é armazenar, organizar, buscar, remover, analisar e comparar dados de telemetria utilizando diferentes estruturas de dados. O sistema também executa benchmarks, testes com restrições, operações adicionais sobre o dataset, comparação entre uma estrutura original e uma estrutura otimizada, além de oferecer visualização interativa por terminal.

---

## 2. Objetivos

O sistema tem como objetivos principais:

- carregar um dataset de telemetria GPS em formato CSV;
- validar e tratar os dados carregados;
- implementar cinco estruturas de dados;
- permitir inserção, busca e remoção;
- executar três operações adicionais sobre os dados;
- realizar benchmarks comparativos;
- simular cinco restrições exigidas pela proposta;
- implementar uma estrutura otimizada e compará-la com a original;
- gerar arquivos CSV com os resultados dos testes;
- disponibilizar um menu interativo no terminal.

---

## 3. Estrutura de Pastas

A organização final do projeto é:

```text
ESTRUTURA-DE-DADOS/
│
├── src/
│   └── main.cpp
│
├── tools/
│   └── gerador_dataset.cpp
│
├── data/
│   └── dataset_telemetria.csv
│
├── results/
│   ├── resultados_benchmark.csv
│   ├── resultados_restricoes.csv
│   └── comparacao_hash_otimizada.csv
│
├── docs/
│   ├── relatorio.md
│   ├── relatorio.pdf
│   └── uso_ia.md
│
├── bin/
│   └── sistema
│
├── README.md
└── .gitignore
```

---

## 4. Dataset

O dataset utilizado fica localizado em:

```text
data/dataset_telemetria.csv
```

Cada registro do dataset segue o formato:

```text
timestamp,latitude,longitude,elevacao,velocidade
```

O sistema exige pelo menos 10.000 registros válidos, conforme a proposta do trabalho. Durante a leitura, o programa descarta registros inválidos, como:

- latitude fora do intervalo `[-90, 90]`;
- longitude fora do intervalo `[-180, 180]`;
- elevação fora da faixa física definida no sistema;
- velocidade negativa;
- valores inválidos, infinitos ou não numéricos.

Após a leitura, os registros são ordenados por timestamp e timestamps duplicados são removidos.

---

## 5. Estruturas de Dados Implementadas

O projeto implementa cinco estruturas de dados, sendo três tradicionais e duas fora da ementa.

### 5.1 Estruturas Tradicionais

| Estrutura | Finalidade |
|---|---|
| Lista Encadeada | Armazenamento sequencial e comparação com estruturas mais eficientes |
| Árvore AVL | Indexação e busca ordenada por timestamp |
| Tabela Hash | Busca rápida por timestamp |

### 5.2 Estruturas Fora da Ementa

| Estrutura | Finalidade |
|---|---|
| Fenwick Tree | Consulta eficiente de soma acumulada de ganho de elevação |
| KD-Tree | Busca espacial por latitude e longitude |

---

## 6. Operações Básicas

As estruturas foram utilizadas para atender às operações básicas exigidas:

- inserção;
- busca;
- remoção.

Observações:

- na Fenwick Tree, a remoção é feita por atualização do valor para zero;
- na KD-Tree, a remoção é lógica, marcando o nó como inativo;
- Lista Encadeada, AVL e Tabela Hash possuem inserção, busca e remoção explícitas por timestamp.

---

## 7. Operações Adicionais

Além de inserção, busca e remoção, o sistema executa três operações adicionais sobre os dados:

### 7.1 Estatísticas

São calculadas estatísticas sobre velocidade e elevação:

- média;
- menor valor;
- maior valor;
- desvio padrão.

### 7.2 Detecção e Filtragem de Anomalias

O sistema detecta registros suspeitos com base em:

- velocidade acima de um limite calculado;
- salto brusco de elevação entre registros consecutivos.

Também é feita filtragem de registros considerados normais.

### 7.3 Classificação de Movimento

Cada registro é classificado de acordo com a velocidade:

| Classe | Critério |
|---|---|
| Parado | velocidade menor que 1 |
| Lento | velocidade entre 1 e 10 |
| Moderado | velocidade entre 10 e 30 |
| Rápido | velocidade maior ou igual a 30 |

---

## 8. Benchmarks

O sistema executa benchmarks medindo:

- tempo de inserção;
- tempo de busca;
- tempo de remoção;
- uso estimado de memória;
- colisões na Tabela Hash;
- escalabilidade com diferentes tamanhos de entrada.

Os resultados são salvos em:

```text
results/resultados_benchmark.csv
```

As comparações são feitas respeitando a finalidade de cada estrutura:

- Lista Encadeada, AVL e Tabela Hash são comparadas em busca por timestamp;
- Fenwick Tree é avaliada separadamente em consultas de soma acumulada;
- KD-Tree é avaliada separadamente em busca espacial.

Essa separação evita comparações inadequadas entre estruturas com finalidades diferentes.

---

## 9. Testes com Restrições

O sistema implementa cinco testes com restrições, contemplando diferentes categorias da proposta.

| Categoria | Restrição | Descrição |
|---|---|---|
| Memória | R5 | Descarte automático de dados antigos |
| Processamento | R7 | Limite máximo de tempo por operação |
| Latência | R13 | Processamento em lote com atraso artificial |
| Dados | R16 | Perda de 20% das leituras |
| Algorítmica/Estrutural | R21 | Substituição de estrutura eficiente por estrutura menos eficiente |

Os resultados são salvos em:

```text
results/resultados_restricoes.csv
```

---

## 10. Estrutura Otimizada

A estrutura escolhida para otimização foi a Tabela Hash.

### 10.1 Tabela Hash Original

A versão original utiliza:

- encadeamento separado;
- nós alocados dinamicamente;
- ponteiros;
- tratamento de colisões por listas encadeadas.

### 10.2 Tabela Hash Otimizada

A versão otimizada utiliza:

- vetores contíguos;
- endereçamento aberto;
- sondagem linear;
- marcação de posições vazias, ocupadas e removidas;
- melhor localidade de memória;
- redução do uso de ponteiros.

A comparação entre a versão original e a versão otimizada é salva em:

```text
results/comparacao_hash_otimizada.csv
```

---

## 11. Visualização Interativa

O sistema possui um menu interativo no terminal:

```text
1 - Exibir status do sistema
2 - Exibir primeiros registros
3 - Buscar registro por timestamp usando Hash
4 - Buscar ponto geográfico mais próximo usando KD-Tree
5 - Consultar ganho de elevação usando Fenwick Tree
6 - Exibir operações adicionais
7 - Exibir arquivos CSV gerados
0 - Sair
```

Esse menu permite explorar os dados, executar consultas e visualizar resultados diretamente no terminal.

---

## 12. Como Compilar

A partir da pasta raiz do projeto, execute:

```bash
g++ -std=c++17 -Wall -Wextra src/main.cpp -o bin/sistema
```

---

## 13. Como Executar

Execute o programa a partir da raiz do projeto:

```bash
./bin/sistema
```

É importante executar a partir da pasta raiz, pois o programa utiliza os caminhos:

```text
data/dataset_telemetria.csv
results/resultados_benchmark.csv
results/resultados_restricoes.csv
results/comparacao_hash_otimizada.csv
```

---

## 14. Como Gerar o Dataset

Caso seja necessário gerar novamente o dataset sintético, compile o gerador:

```bash
g++ -std=c++17 -Wall -Wextra tools/gerador_dataset.cpp -o bin/gerador
```

Execute:

```bash
./bin/gerador
```

O arquivo gerado deve ser colocado em:

```text
data/dataset_telemetria.csv
```

---

## 15. Arquivos Gerados

Durante a execução, o sistema gera ou atualiza:

```text
results/resultados_benchmark.csv
results/resultados_restricoes.csv
results/comparacao_hash_otimizada.csv
```

Esses arquivos podem ser usados para análise, tabelas e gráficos.

---

## 16. Complexidade Teórica Resumida

| Estrutura | Inserção | Busca | Remoção | Observação |
|---|---|---|---|---|
| Lista Encadeada | O(1) no fim | O(n) | O(n) | Simples, mas pouco eficiente para busca |
| AVL | O(log n) | O(log n) | O(log n) | Mantém balanceamento |
| Tabela Hash | O(1) médio | O(1) médio | O(1) médio | Pode sofrer colisões |
| Fenwick Tree | O(log n) | O(log n) para soma prefixada | O(log n) por atualização | Usada para somas acumuladas |
| KD-Tree | O(log n) médio | O(log n) médio para busca espacial | Remoção lógica | Usada para dados multidimensionais |

---

## 17. Observações Metodológicas

Nem todas as estruturas foram comparadas diretamente entre si, pois possuem finalidades diferentes.

A comparação direta foi feita principalmente entre:

- Lista Encadeada;
- AVL;
- Tabela Hash.

Essas estruturas foram comparadas em operações de busca por timestamp.

A Fenwick Tree foi avaliada separadamente, pois sua finalidade principal é consulta acumulada de intervalos.

A KD-Tree foi avaliada separadamente, pois sua finalidade principal é busca espacial por latitude e longitude.

---

## 18. Uso de IA Generativa

Durante o desenvolvimento do código, foram utilizados recursos de IA generativa como apoio para:

- análise dos requisitos do projeto;
- organização modular do código;
- implementação incremental das estruturas;
- correção de erros de compilação;
- criação de testes;
- organização da estrutura de pastas;
- apoio à documentação técnica.

Os trechos de código desenvolvidos com apoio de IA devem ser registrados no arquivo:

```text
docs/uso_ia.md
```

O relatório técnico final deve ser escrito pelo aluno, conforme a regra da proposta.

---

## 19. Documentação

A documentação do projeto fica em:

```text
docs/
```

Arquivos previstos:

| Arquivo | Finalidade |
|---|---|
| docs/relatorio.md | Base editável do relatório técnico |
| docs/relatorio.pdf | Relatório técnico final em PDF |
| docs/uso_ia.md | Registro de uso de IA generativa |

---

## 20. Autor

Nome: Paulo Sérgio Magri Júnior 
Curso: Engenharia de Controle e Automação  
Disciplina: Estrutura de Dados  
Ano: 2026