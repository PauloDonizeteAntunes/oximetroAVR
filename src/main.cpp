#include "../lib/funsape/funsapeLibGlobalDefines.hpp"
#include "../lib/funsape/peripheral/funsapeLibUsart0.hpp"
#include "../lib/funsape/peripheral/funsapeLibInt0.hpp"
#include "../lib/MAX30102.h"
#include "../lib/twi_master.h"

volatile bool fifo_rdy = 0;

#define BUFFER_SIZE 100
#define MIN_PEAK_DISTANCE 10  // Mínima distância entre picos (em amostras)
#define PEAK_THRESHOLD 20     // Threshold para detecção de pico

// Buffer circular para armazenar amostras
static uint32_t sample_buffer[BUFFER_SIZE];
static uint8_t buffer_index = 0;
static bool buffer_full = false;

// Array para armazenar intervalos entre batimentos
static uint16_t beat_intervals[10];
static uint8_t interval_index = 0;
static uint32_t last_peak_time = 0;

void usartConfg(){
    usart0.setBaudRate(Usart0::BaudRate::BAUD_RATE_57600);
    usart0.init();               // primeiro init
    usart0.enableTransmitter();  // depois habilita TX
    usart0.stdio();              // só agora vincula printf() ao usart0
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
    printf("PART_ID: 0x%02X\n", part_id);

    if (part_id != 0x15) { // Expected PART_ID for MAX30102
        printf("Error: Invalid PART_ID. Expected 0x15, got 0x%02X\n", part_id);
        return false;
    }

    //setup
    writeRegister(MAX30102_MODE_CONFIG, MAX30102_MODE_RESET);
    writeRegister(MAX30102_MODE_CONFIG, MAX30102_MODE_HEART_RATE);
    delayMs(100);
    printf("setup de coracao\n");


    //Sim e spo02 que configura o samplerate de todo mundo
    uint8_t spo2_config = MAX30102_SPO2_ADC_RGE_4096 |
                         MAX30102_SPO2_SR_800 |
                         MAX30102_SPO2_PW_411;
    writeRegister(MAX30102_SPO2_CONFIG, spo2_config);
    printf("setup de config\n");


    //Configura avg da fifo e habilita INT0 para quando a fifo estiver com X samples configuradas
    uint8_t fifo_config = MAX30102_SAMPLEAVG_1 |       // Sem averaging adicional no FIFO
                         MAX30102_ROLLOVER_EN |         // Habilita rollover
                         0x02;                          // 30 amostra levanta INT0
    writeRegister(MAX30102_FIFO_CONFIG, fifo_config);
    printf("setup de FIFO\n");

    // IMPORTANTE: Configurar interrupções do MAX30102 - APENAS FIFO Almost Full
    writeRegister(MAX30102_INT_ENABLE_1, MAX30102_INT_A_FULL);  // Apenas FIFO Almost Full
    writeRegister(MAX30102_INT_ENABLE_2, 0x00);                 // Não usar temperatura
    printf("setup de interrupcoes\n");

    // Limpar quaisquer interrupções pendentes lendo os registradores de status
    readRegister(MAX30102_INT_STATUS_1);
    readRegister(MAX30102_INT_STATUS_2);
    printf("interrupcoes limpas\n");

    //Red led config
    writeRegister(MAX30102_LED1_PA, 0x1F);
    writeRegister(MAX30102_LED2_PA, 0x1F);
    printf("setup de led\n");

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

volatile unsigned long timer0_millis = 0;
volatile unsigned long timer0_overflow_count = 0;

ISR(TIMER0_COMPA_vect) {
    timer0_millis++;
}

unsigned long millis() {
    unsigned long m;

    uint8_t oldSREG = SREG;   // Salva o status de interrupções
    cli();                    // Desabilita interrupções
    m = timer0_millis;        // Lê o contador
    SREG = oldSREG;           // Restaura o status de interrupções

    return m;
}

bool checkForBeat(int32_t sample) {
    static int32_t prevSample = 0;
    static int32_t thresh = 20000;
    static bool rising = false;
    static uint32_t lastBeatTime = 0;
    static int32_t maxValue = 0;
    static int32_t minValue = 65535;
    static uint16_t sampleCount = 0;
    static int32_t peakValue = 0;
    static uint8_t risingCount = 0;  // Contador para garantir subida consistente

    bool beatDetected = false;
    uint32_t now = millis();

    // Atualiza valores min/max
    if (sample > maxValue) maxValue = sample;
    if (sample < minValue) minValue = sample;

    sampleCount++;

    // Ajusta threshold a cada 50 amostras
    if (sampleCount % 50 == 0) {
        thresh = minValue + ((maxValue - minValue) * 10 / 100); // 75% para ser mais seletivo
    }

    if (sample > thresh) {
        if (!rising && (sample - prevSample) > 100) {  // Subida mais significativa
            rising = true;
            peakValue = sample;
            risingCount = 1;
        }

        // Conta amostras consecutivas subindo
        if (rising && sample > prevSample) {
            risingCount++;
            if (sample > peakValue) {
                peakValue = sample;
            }
        }

        // Detecta quando começa a descer após subida consistente
        if (rising && risingCount >= 3 && (sample < peakValue - 300)) {  // Pelo menos 3 amostras subindo e queda de 300
            // Intervalo mínimo de 750ms (máximo 80 BPM)
            if ((now - lastBeatTime) >= 750) {
                lastBeatTime = now;
                beatDetected = true;
            }
            rising = false;
            peakValue = 0;
            risingCount = 0;
        }
    } else {
        rising = false;
        peakValue = 0;
        risingCount = 0;
    }

    prevSample = sample;
    return beatDetected;
}

int main(void)
{
    TCCR0A = (1 << WGM01);                      // Modo CTC
    TCCR0B = (1 << CS01) | (1 << CS00);         // Prescaler 64
    OCR0A = 249;                                // Valor de comparação
    TIMSK0 |= (1 << OCIE0A);                    // Habilita interrupção

    usartConfg();

    if (!initMAX30102()) {
        printf("Failed to initialize MAX30102\n");
        return -1;
    }

    //init inicialize
    setBit(PORTD, PD2);
    int0.init(Int0::SenseMode::FALLING_EDGE);
    int0.clearInterruptRequest();
    int0.activateInterrupt();

    sei();

    uint32_t lastBeat = 0;  // fora do while(1)

    while(1){
        if (fifo_rdy) {

            uint8_t available = getAvailableSamples();
            uint32_t red_samples[32];  // Buffer para amostras

            if (available > 0) {

                // Ler todas as amostras disponíveis de uma vez
                for (uint8_t i = 0; i < available; i++) {
                    uint32_t red, ir;
                    readFIFO(&red, &ir);
                    red_samples[i] = red;
                    // printf("Red: %lu, IR: %lu\n", red, ir);

                if (checkForBeat(ir)) {
                    printf("Temos um beat\n");

                    uint32_t now = millis();
                    uint32_t delta = now - lastBeat;
                    lastBeat = now;

                if (delta > 100 && delta < 2500) { // Apenas BPM entre 30-200
                    uint32_t bpm = 60000UL / delta;
                    printf("BPM %lu (delta: %lums)\n", bpm, delta);
                } else {
                    printf("Invalid interval: %lums (ignored)\n", delta);
                }
            }

                //Limpa INT0 para que seja possivel nova setagem do pino
                readRegister(MAX30102_INT_STATUS_1);
                readRegister(MAX30102_INT_STATUS_2);
            }
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