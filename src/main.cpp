#include "../lib/funsape/funsapeLibGlobalDefines.hpp"
#include "../lib/funsape/peripheral/funsapeLibUsart0.hpp"
#include "../lib/max30102/src/MAX30102.h"
#include "../lib/twi_master.h"

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

uint8_t getAvailableSamples() {
    uint8_t write_ptr = readRegister(MAX30102_FIFO_WR_PTR);
    uint8_t read_ptr = readRegister(MAX30102_FIFO_RD_PTR);

    if (write_ptr >= read_ptr) {
        return write_ptr - read_ptr;
    } else {
        return (32 - read_ptr) + write_ptr; // FIFO is 32 samples deep
    }
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
    printf("setup de coracao\n");


    //Sim e spo02 que configura o samplerate de todo mundo
    uint8_t spo2_config = MAX30102_SPO2_ADC_RGE_16384 |
                         MAX30102_SPO2_SR_1000 |
                         MAX30102_SPO2_PW_411;
    writeRegister(MAX30102_SPO2_CONFIG, spo2_config);
    printf("setup de config\n");

    //Red led config
    writeRegister(MAX30102_LED1_PA, 0x60);
    writeRegister(MAX30102_LED2_PA, 0);
    printf("setup de led\n");

    return true;


}


int main(void)
{
    usartConfg();

    if (!initMAX30102()) {
        printf("Failed to initialize MAX30102\n");
        return -1;
    }

    while(1){
        uint32_t red, ir;
        readFIFO(&red, &ir);

        // if (ir < 50000){
        //     printf(" No finger?\n");
        //     continue;
        // }

        printf("Red: %lu, IR: %lu\n", red, ir);

        delayMs(10);
    }

    return 0;
}