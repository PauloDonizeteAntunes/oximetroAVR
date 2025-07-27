#include "../lib/funsape/funsapeLibGlobalDefines.hpp"
#include "../lib/funsape/peripheral/funsapeLibUsart0.hpp"
#include "../lib/funsape/peripheral/funsapeLibInt0.hpp"
#include "../lib/MAX30102.h"
#include "../lib/twi_master.h"

volatile bool fifo_rdy = 0;

void usartConfg(){
    usart0.setBaudRate(Usart0::BaudRate::BAUD_RATE_57600);
    usart0.init();               // primeiro init
    usart0.enableTransmitter();  // depois habilita TX
    usart0.stdio();              // só agora vincula // printf() ao usart0
}

void writeRegister(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    tw_master_transmit(MAX30102_I2C_ADDRESS, data, 2, false);
}

// Ler registrador
uint8_t readRegister(uint8_t reg) {
    uint8_t value;
    tw_master_transmit(MAX30102_I2C_ADDRESS, &reg, 1, true);
    tw_master_receive(MAX30102_I2C_ADDRESS, &value, 1);
    return value;
}

void readFIFO(uint32_t* red, uint32_t* ir) {
    uint8_t reg = MAX30102_FIFO_DATA;
    uint8_t fifo_data[6]; // 3 bytes RED + 3 bytes IR

    // Point to FIFO data register
    tw_master_transmit(MAX30102_I2C_ADDRESS, &reg, 1, true);
    // Read 6 bytes (even in heart rate mode, read full sample)
    tw_master_receive(MAX30102_I2C_ADDRESS, fifo_data, 6);

    // Reconstruct 18-bit values
    *red = ((uint32_t)fifo_data[0] << 16) | ((uint32_t)fifo_data[1] << 8) | fifo_data[2];
    *red &= 0x03FFFF;  // Mask para 18 bits

    *ir = ((uint32_t)fifo_data[3] << 16) | ((uint32_t)fifo_data[4] << 8) | fifo_data[5];
    *ir &= 0x03FFFF;   // Mask para 18 bits
}


bool initMAX30102() {

        // Inicializar TWI
    tw_init(TW_FREQ_400K, false);

    // Endereço do MAX30102

    // Ler PART_ID (registrador 0xFF)
    uint8_t reg_addr = 0xFF;
    uint8_t part_id;

    tw_master_transmit(MAX30102_I2C_ADDRESS, &reg_addr, 1, true);
    delayMs(1);
    tw_master_receive(MAX30102_I2C_ADDRESS, &part_id, 1);
    delayMs(1);
    // printf("PART_ID: 0x%02X\n", part_id);

    if (part_id != 0x15) { // Expected PART_ID for MAX30102
        // printf("Error: Invalid PART_ID. Expected 0x15, got 0x%02X\n", part_id);
        return false;
    }

    //setup
    writeRegister(MAX30102_MODE_CONFIG, MAX30102_MODE_RESET);
    writeRegister(MAX30102_MODE_CONFIG, MAX30102_MODE_HEART_RATE);
    delayMs(100);
    // printf("setup de coracao\n");


    //Sim e spo02 que configura o samplerate de todo mundo
    uint8_t spo2_config = MAX30102_SPO2_ADC_RGE_16384 |
                         MAX30102_SPO2_SR_1000 |
                         MAX30102_SPO2_PW_215;
    writeRegister(MAX30102_SPO2_CONFIG, spo2_config);
    // printf("setup de config\n");


    //Configura avg da fifo e habilita INT0 para quando a fifo estiver com X samples configuradas
    uint8_t fifo_config = MAX30102_SAMPLEAVG_1 |    // Sem averaging adicional no FIFO
                         MAX30102_ROLLOVER_EN |         // Habilita rollover
                         0x02;                          // 30 amostra levanta INT0
    writeRegister(MAX30102_FIFO_CONFIG, fifo_config);
    // printf("setup de FIFO\n");

    // IMPORTANTE: Configurar interrupções do MAX30102 - APENAS FIFO Almost Full
    writeRegister(MAX30102_INT_ENABLE_1, MAX30102_INT_A_FULL);  // Apenas FIFO Almost Full
    writeRegister(MAX30102_INT_ENABLE_2, 0x00);                 // Não usar temperatura
    // printf("setup de interrupcoes\n");

    // Limpar quaisquer interrupções pendentes lendo os registradores de status
    readRegister(MAX30102_INT_STATUS_1);
    readRegister(MAX30102_INT_STATUS_2);
    // printf("interrupcoes limpas\n");

    //Red led config
    writeRegister(MAX30102_LED1_PA, 0x60);
    writeRegister(MAX30102_LED2_PA, 0x60);
    // printf("setup de led\n");

    return true;


}

uint8_t getAvailableSamples() {
    uint8_t write_ptr = readRegister(MAX30102_FIFO_WR_PTR);
    uint8_t read_ptr = readRegister(MAX30102_FIFO_RD_PTR);

    if (write_ptr >= read_ptr) {
        return write_ptr - read_ptr;
    } else {
        return (32 - read_ptr) + write_ptr; // FIFO is 32 samples deep
    }
}

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

// Configurações do BPM
#define SAMPLE_RATE 62.5f
#define MAX_VALES 300
#define VARIACAO_MINIMA 170
#define QUEDA_ADICIONAL 100  // tolerância para continuação da queda

uint8_t detectar_vales_queda_e_subida(const uint32_t* dados, uint16_t tamanho, uint16_t* indices_vales, uint16_t max_vales) {
    uint8_t total_vales = 0;
    uint16_t i = 1;

    while (i < tamanho - 1 && total_vales < max_vales) {
        // Verifica se houve uma queda inicial
        if (dados[i] < dados[i - 1]) {
            uint16_t inicio_queda = i - 1;
            uint32_t valor_inicio = dados[inicio_queda];

            // Continua descendo para achar o ponto mais baixo
            uint16_t melhor_vale = i;
            uint32_t menor_valor = dados[i];

            while (i < tamanho && dados[i] < dados[i - 1]) {
                if (dados[i] < menor_valor) {
                    menor_valor = dados[i];
                    melhor_vale = i;
                }
                i++;
            }

            int32_t variacao_queda = valor_inicio - menor_valor;
            if (variacao_queda >= VARIACAO_MINIMA) {
                // Aguarda a subida de pelo menos VARIACAO_MINIMA
                uint32_t valor_mais_baixo = menor_valor;

                while (i < tamanho && dados[i] < valor_mais_baixo + VARIACAO_MINIMA) {
                    if (dados[i] < valor_mais_baixo) {
                        valor_mais_baixo = dados[i];
                        melhor_vale = i; // ainda caindo
                    }
                    i++;
                }

                // Quando subir o suficiente, considera esse vale
                indices_vales[total_vales++] = melhor_vale;
            }
        } else {
            i++;
        }
    }

    return total_vales;
}

float calcular_bpm(const uint16_t* indices_vales, uint8_t total_vales, float fs_amostragem) {
    if (total_vales < 2) return 0.0f;

    float soma_intervalos = 0.0f;

    for (uint8_t i = 1; i < total_vales; i++) {
        uint16_t delta = indices_vales[i] - indices_vales[i - 1];
        float tempo_segundos = delta / fs_amostragem;
        soma_intervalos += tempo_segundos;
    }

    float media_intervalo = soma_intervalos / (total_vales - 1);
    float bpm = 60.0f / media_intervalo;

    return bpm;
}


int main(void)
{

    usartConfg();

    if (!initMAX30102()) {
        // printf("Failed to initialize MAX30102\n");
        return -1;
    }

    //init inicialize
    setBit(PORTD, PD2);
    int0.init(Int0::SenseMode::FALLING_EDGE);
    int0.clearInterruptRequest();
    int0.activateInterrupt();

    sei();

    uint32_t tendeciaRed[3];
    uint32_t tendeciaIr[3];
    uint8_t controle = 16;
    uint8_t x = 0;

    uint32_t bpmAmostra[300];
    uint16_t  bpmIndex = 0;

    const uint32_t tamanho = sizeof(bpmAmostra) / sizeof(bpmAmostra[0]);
    uint16_t indices_vales[MAX_VALES];

    while(1){

        uint8_t available = getAvailableSamples();

        if (fifo_rdy) {

            if (available > 0) {
                for (uint8_t i = 0; i < available; i++) {

                    uint32_t red, ir;
                    uint32_t redMedia, irMedia;

                    for (int i = 0; i < controle; i++)
                    {
                        readFIFO(&red, &ir);

                        irMedia = ir + irMedia;

                    }

                    irMedia = irMedia/controle;
                    tendeciaIr[x] = irMedia;
                    x++;

                    if(x == 3){
                        x = 0;
                        uint32_t retorno1 = calcularTendencia(tendeciaIr);

                        for (uint8_t i = 0; i < 3; ++i) {
                            tendeciaIr[i] = 0;
                        }

                        bpmAmostra[bpmIndex++] = retorno1;
                        if(bpmIndex == 300){
                            uint8_t total_vales = detectar_vales_queda_e_subida(bpmAmostra, tamanho, indices_vales, MAX_VALES);
                                printf("Total de vales detectados: %u\n", total_vales);
                                for (uint8_t i = 0; i < total_vales; i++) {
                                    uint16_t idx = indices_vales[i];
                                    printf("Vale %u: índice %u, valor %u\n", i + 1, idx, bpmAmostra[idx]);
                                }


                              float bpm = calcular_bpm(indices_vales, total_vales, 62.5f);
                              uint16_t parte_inteira = (uint16_t)bpm;
                              uint16_t parte_decimal = (uint16_t)((bpm - parte_inteira) * 100);

                              printf("BPM estimado: %u.%02u\n\r", parte_inteira, parte_decimal);
                          bpmIndex = 0;
                        }
                    }
                }

                // Limpar interrupções
                readRegister(MAX30102_INT_STATUS_1);
                readRegister(MAX30102_INT_STATUS_2);
            }

            int0.clearInterruptRequest();
            fifo_rdy = false;
        }
    }

    return 0;
}

void int0InterruptCallback(void)
{
    fifo_rdy = true;
}