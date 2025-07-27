#include "funsape/funsapeLibGlobalDefines.hpp"


// Configurações
#define MAX_VALES       150
#define MAX_TOP_K       10
#define SAMPLE_RATE 62.5f

//Tratamento de sinal
uint32_t calcularTendencia(const uint32_t valores[3]);

//Saida
uint16_t mediaMaioresVariacoes(const uint32_t* dados, uint16_t tamanho, uint8_t qtdMaiores);
float detectarValesEBPM(const uint32_t* dados, uint16_t tamanho, uint16_t* indices_vales, uint8_t top_variacao);
