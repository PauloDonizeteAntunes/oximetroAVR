//!
//! \file           calcMaster.h
//! \brief          Funções auxiliares para análise de sinal do sensor MAX30102
//! \author         Paulo Donizete Antunes Junior
//! \date           2025-07-30
//! \version        1.0
//! \details        Cálculo de tendência, detecção de vales e cálculo do BPM
//!

#include "../lib/funsape/funsapeLibGlobalDefines.hpp"

// Configurações
#define MAXVALUES       150
#define MAXVARDET       10
#define SAMPLE_RATE     62.5f // Valor fixo desse projeto, calibrado para tal

//Tratamento de sinal
uint32_t calcularTendencia(const uint32_t valores[3]);

//Saida
uint16_t mediaMaioresVariacoes(const uint32_t* dados, uint16_t tamanho, uint8_t qtdMaiores);
float detectarValesEBPM(const uint32_t* dados, uint16_t tamanho, uint16_t* indices_vales, uint8_t top_variacao);
