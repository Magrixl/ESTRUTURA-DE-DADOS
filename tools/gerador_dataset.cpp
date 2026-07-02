#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>

using namespace std;

int main() {
    // Abre o arquivo para escrita
    ofstream arquivo("dataset_telemetria.csv");
    
    if (!arquivo.is_open()) {
        cerr << "Erro ao criar o arquivo CSV!" << endl;
        return 1;
    }

    // Escreve o cabeçalho do CSV
    arquivo << "timestamp,latitude,longitude,elevacao,velocidade\n";

    // Configuração dos geradores de números aleatórios (Ruído)
    random_device rd;
    mt19937 gen(rd());
    
    // Distribuições para simular pequenas variações contínuas
    normal_distribution<double> dist_latlon(0.0, 0.0001); // Ruído na coordenada
    normal_distribution<double> dist_elev(0.0, 0.5);      // Variação de elevação (metros)
    normal_distribution<double> dist_vel(0.0, 1.2);       // Variação de velocidade (km/h)

    // Valores iniciais (Ponto de partida em Sorocaba)
    uint64_t timestamp = 1710000000000; // Epoch fictício inicial
    double lat_atual = -23.5015;
    double lon_atual = -47.4581;
    double elevacao_atual = 600.0;
    double velocidade_atual = 15.0; // Velocidade média inicial

    int num_amostras = 15000; // Acima do mínimo exigido de 10.000

    cout << "Gerando " << num_amostras << " registros de telemetria..." << endl;

    for (int i = 0; i < num_amostras; i++) {
        // Grava a linha atual no arquivo
        arquivo << fixed << setprecision(6) 
                << timestamp << ","
                << lat_atual << ","
                << lon_atual << ","
                << setprecision(2) << elevacao_atual << ","
                << velocidade_atual << "\n";

        // Simula o avanço do tempo (1 segundo por leitura = 1000 ms)
        timestamp += 1000;

        // Atualiza os valores com um "random walk" (passeio aleatório suave)
        lat_atual += dist_latlon(gen);
        lon_atual += dist_latlon(gen);
        
        elevacao_atual += dist_elev(gen);
        // Evita elevação negativa extrema
        if (elevacao_atual < 500.0) elevacao_atual = 500.0; 

        velocidade_atual += dist_vel(gen);
        // Mantém a velocidade dentro de limites realistas para MTB (0 a 45 km/h)
        if (velocidade_atual < 0.0) velocidade_atual = 0.0;
        if (velocidade_atual > 45.0) velocidade_atual = 45.0;
    }

    arquivo.close();
    cout << "Arquivo 'dataset_telemetria.csv' gerado com sucesso!" << endl;

    return 0;
}
