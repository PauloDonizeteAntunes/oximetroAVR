#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// Definições de tipos para economizar memória
typedef int32_t data_point_t;    // Dados do sensor (-32768 a 32767)
typedef uint8_t sample_index_t;  // Índice de amostra (0-255, ajuste conforme necessário)
typedef float float_t;           // Tipo float

// Configurações do sistema
#define MAX_SAMPLES 200          // Máximo de amostras que podemos armazenar
#define MAX_PEAKS 20             // Máximo de picos detectáveis
#define MIN_PEAK_HEIGHT 10000    // Altura mínima para considerar um pico R
#define SAMPLING_FREQUENCY 62.5f // Frequência de amostragem em Hz

// Buffers de dados em RAM
static data_point_t channel_data[MAX_SAMPLES];
static sample_index_t peak_buffer[MAX_PEAKS];

// Contador de amostras
static uint8_t sample_count = 0;

// Função para carregar dados passados como parâmetro
void load_sample_data(uint32_t* sample_data, uint8_t data_size) {
    sample_count = (data_size > MAX_SAMPLES) ? MAX_SAMPLES : data_size;

    for (uint8_t i = 0; i < sample_count; i++) {
        channel_data[i] = sample_data[i];
    }
}

// Função para carregar dados da PROGMEM
void load_sample_data_progmem(uint32_t* sample_data, uint8_t data_size) {
    sample_count = (data_size > MAX_SAMPLES) ? MAX_SAMPLES : data_size;

    for (uint8_t i = 0; i < sample_count; i++) {
        channel_data[i] = pgm_read_word(&sample_data[i]);
    }
}

// Função otimizada para encontrar picos
uint8_t find_peaks(const data_point_t* data, uint8_t data_size,
                   sample_index_t* peaks, float_t min_distance_samples) {

    uint8_t peak_count = 0;

    if (data_size < 3) return 0;  // Precisa de pelo menos 3 pontos

    uint8_t peak_width_samples = (uint8_t)(min_distance_samples / 5.0f);
    if (peak_width_samples < 2) peak_width_samples = 2;

    sample_index_t last_peak_idx = 0;

    for (uint8_t i = peak_width_samples; i < (data_size - peak_width_samples); i++) {
        // Verifica se está acima do limiar mínimo
        if (data[i] > MIN_PEAK_HEIGHT) {
            uint8_t is_local_max = 1;

            // Verifica janela à direita
            for (uint8_t j = 1; j <= peak_width_samples; j++) {
                if (data[i + j] > data[i]) {
                    is_local_max = 0;
                    break;
                }
            }

            // Verifica janela à esquerda (só se ainda for máximo local)
            if (is_local_max) {
                for (uint8_t j = 1; j <= peak_width_samples; j++) {
                    if (data[i - j] > data[i]) {
                        is_local_max = 0;
                        break;
                    }
                }
            }

            // Se é pico local e respeitou a distância mínima
            if (is_local_max && ((i - last_peak_idx) >= min_distance_samples)) {
                if (peak_count < MAX_PEAKS) {
                    peaks[peak_count] = i;
                    peak_count++;
                    last_peak_idx = i;
                }
            }
        }
    }

    return peak_count;
}

float process_bpm_data(const uint32_t *data, uint16_t data_size) {
    const float min_distance_sec = 0.5f;
    const uint16_t min_distance_samples = (uint16_t)(SAMPLING_FREQUENCY * min_distance_sec);

    const uint32_t min_peak_prominence = 10;  // Ajuste conforme necessário
    const uint32_t min_peak_height = 30800;   // Ajuste conforme seu sinal

    uint16_t peaks[64];  // Guardar no máximo 64 picos
    uint16_t peak_count = 0;
    uint16_t last_peak_index = 0;

    for (uint16_t i = 1; i < data_size - 1; ++i) {
        uint32_t left = data[i - 1];
        uint32_t right = data[i + 1];
        uint32_t min_neighbor = (left < right) ? left : right;

        if (data[i] > left &&
            data[i] > right &&
            data[i] > min_peak_height &&
            (data[i] - min_neighbor) >= min_peak_prominence) {

            if (peak_count == 0 || (i - last_peak_index) >= min_distance_samples) {
                peaks[peak_count++] = i;
                last_peak_index = i;
            }
        }
    }

    if (peak_count < 2) {
        return 0.0f;  // Ou NAN se preferir
    }

    float total_interval = 0.0f;
    for (uint16_t i = 1; i < peak_count; ++i) {
        total_interval += (float)(peaks[i] - peaks[i - 1]);
    }

    float avg_interval = total_interval / (peak_count - 1);
    float bpm = 60.0f * SAMPLING_FREQUENCY / avg_interval;

    return bpm;
}