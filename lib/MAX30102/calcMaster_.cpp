//!
//! \file           calcMaster.cpp
//! \brief          Funções auxiliares para análise de sinal do sensor MAX30102
//! \author         Paulo Donizete Antunes Junior
//! \date           2025-07-30
//! \version        1.0
//! \details        Cálculo de tendência, detecção de vales e cálculo do BPM
//!

#include "calcMaster.h"

// -----------------------------------------------------------------------------
// Função auxiliar: calcula tendência com base em três valores consecutivos
// -----------------------------------------------------------------------------

uint32_t calcularTendencia(const uint32_t valores[3]) {
    uint32_t a = valores[0];
    uint32_t b = valores[1];
    uint32_t c = valores[2];

    int8_t ab = (b > a) ? 1 : (b < a) ? -1 : 0;
    int8_t bc = (c > b) ? 1 : (c < b) ? -1 : 0;

    if (ab == bc && ab != 0) {
        return c;
    }

    if (ab != 0 && bc != 0 && ab != bc) {
        return (bc == ab) ? b : c;
    }

    if (ab == 0 && bc == 0) return c;
    if (ab == 0) return c;
    if (bc == 0) return c;

    return c;
}

// -----------------------------------------------------------------------------
// Função auxiliar: calcula média das maiores variações absolutas entre amostras
// -----------------------------------------------------------------------------
uint16_t mediaMaioresVariacoes(const uint32_t* dados, uint16_t tamanho, uint8_t qtdMaiores) {
    if (tamanho < 2 || qtdMaiores == 0) return 0;

    const uint8_t k = (qtdMaiores < MAXVARDET) ? qtdMaiores : MAXVARDET;
    uint32_t top[MAXVARDET];
    uint8_t count = 0;

    // Primeira iteração para inicializar
    uint32_t prev = dados[0];

    for (uint16_t i = 1; i < tamanho; ++i) {
        const uint32_t curr = dados[i];
        const uint32_t diff = (curr > prev) ? (curr - prev) : (prev - curr);

        // Inserção por inserção binária otimizada
        if (count < k) {
            // Array ainda não está cheio
            uint8_t pos = count;
            while (pos > 0 && top[pos - 1] < diff) {
                top[pos] = top[pos - 1];
                --pos;
            }
            top[pos] = diff;
            ++count;
        } else if (diff > top[k - 1]) {
            // Substituir o menor valor se necessário
            uint8_t pos = k - 1;
            while (pos > 0 && top[pos - 1] < diff) {
                top[pos] = top[pos - 1];
                --pos;
            }
            top[pos] = diff;
        }

        prev = curr;
    }

    if (count == 0) return 0;

    // Soma otimizada com acumulador
    uint32_t soma = 0;
    const uint8_t* end = &count;
    for (uint8_t i = 0; i < *end; ++i) {
        soma += top[i];
    }

    // Divisão otimizada para potências de 2 comuns
    return (count == 1) ? soma :
           (count == 2) ? (soma >> 1) :
           (count == 4) ? (soma >> 2) :
           (count == 8) ? (soma >> 3) :
           soma / count;
}

// -----------------------------------------------------------------------------
// Função principal: detecta vales e calcula BPM
// -----------------------------------------------------------------------------
float detectarValesEBPM(const uint32_t* dados, uint16_t tamanho, uint16_t* indices_vales, uint8_t top_variacao) {
    if (tamanho < 3) return 0.0f;

    const uint16_t varMinima = mediaMaioresVariacoes(dados, tamanho, top_variacao);
    if (varMinima == 0) return 0.0f;

    uint8_t total_vales = 0;
    uint16_t i = 1;
    const uint16_t tamanho_menos_1 = tamanho - 1;

    // Cache dos valores para reduzir acessos à memória
    uint32_t val_anterior = dados[0];
    uint32_t val_atual = dados[1];

    while (i < tamanho_menos_1 && total_vales < MAXVALUES) {
        const uint32_t val_proximo = dados[i + 1];

        // Verificação de vale: val_atual < val_anterior
        if (val_atual < val_anterior) {
            const uint16_t inicio = i - 1;
            const uint32_t valor_inicio = val_anterior;

            uint16_t melhor_vale = i;
            uint32_t menor_valor = val_atual;

            // Encontrar o vale mais profundo na sequência descendente
            ++i;
            val_anterior = val_atual;
            val_atual = val_proximo;

            while (i < tamanho && val_atual < val_anterior) {
                if (val_atual < menor_valor) {
                    menor_valor = val_atual;
                    melhor_vale = i;
                }
                ++i;
                if (i < tamanho) {
                    val_anterior = val_atual;
                    val_atual = dados[i];
                }
            }

            // Verificar se a variação é significativa
            const uint32_t variacao = valor_inicio - menor_valor;
            if (variacao >= varMinima) {
                // Esperar subida mínima para confirmar o vale
                const uint32_t threshold = menor_valor + varMinima;
                while (i < tamanho && val_atual < threshold) {
                    if (val_atual < menor_valor) {
                        menor_valor = val_atual;
                        melhor_vale = i;
                    }
                    ++i;
                    if (i < tamanho) {
                        val_anterior = val_atual;
                        val_atual = dados[i];
                    }
                }
                indices_vales[total_vales++] = melhor_vale;
            }

            // Atualizar cache se ainda dentro dos limites
            if (i < tamanho) {
                if (i > 0) val_anterior = dados[i - 1];
                val_atual = dados[i];
            }
        } else {
            ++i;
            val_anterior = val_atual;
            if (i < tamanho) val_atual = dados[i];
        }
    }

    if (total_vales < 2) return 0.0f;

    // Cálculo do BPM otimizado
    float soma_intervalos = 0.0f;
    const float inv_sample_rate = 1.0f / SAMPLE_RATE;

    // Loop unrolling para casos comuns
    uint8_t j = 1;
    const uint8_t total_menos_1 = total_vales - 1;

    // Processar pares quando possível (loop unrolling 2x)
    for (; j < total_menos_1; j += 2) {
        const uint16_t delta1 = indices_vales[j] - indices_vales[j - 1];
        const uint16_t delta2 = indices_vales[j + 1] - indices_vales[j];
        soma_intervalos += (delta1 + delta2) * inv_sample_rate;
    }

    // Processar elemento restante se ímpar
    if (j < total_vales) {
        const uint16_t delta = indices_vales[j] - indices_vales[j - 1];
        soma_intervalos += delta * inv_sample_rate;
    }

    const float media_intervalo = soma_intervalos / (total_vales - 1);
    return (media_intervalo > 0.0f) ? (60.0f / media_intervalo) : 0.0f;
}
