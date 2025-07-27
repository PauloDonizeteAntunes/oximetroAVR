#include "MAX30102.h"
#include "twi_master.h"
#include "funsape/funsapeLibGlobalDefines.hpp"
#include "funsape/peripheral/funsapeLibInt0.hpp"

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


    //init inicialize
    setBit(PORTD, PD2);
    int0.init(Int0::SenseMode::FALLING_EDGE);
    int0.clearInterruptRequest();
    int0.activateInterrupt();

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