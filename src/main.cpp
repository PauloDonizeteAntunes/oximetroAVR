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
    writeRegister(MAX30102_LED1_PA, 0x1F);
    writeRegister(MAX30102_LED2_PA, 0x1F);
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
#define MAX_PEAKS 20
#define SAMPLE_RATE 62.5f
#define MIN_DISTANCE_SAMPLES 31
#define ARRAY_SIZE 625



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

                        printf("%lu\n\r", retorno1);

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