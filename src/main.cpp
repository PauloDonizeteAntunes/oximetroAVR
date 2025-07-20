#include "../lib/funsape/funsapeLibGlobalDefines.hpp"
#include "../lib/funsape/peripheral/funsapeLibUsart0.hpp"
#include "../lib/max30102/src/MAX30102.h"
#include "../lib/twi_master.h"

void usartConfg(){
    usart0.setBaudRate(Usart0::BaudRate::BAUD_RATE_9600);
    usart0.init();               // primeiro init
    usart0.enableTransmitter();  // depois habilita TX
    usart0.stdio();              // só agora vincula printf() ao usart0
}

int main(void)
{
    usartConfg();

    // Inicializar TWI
    tw_init(TW_FREQ_400K, true);

    // Endereço do MAX30102
    uint8_t max30102_addr = 0x57;

    // Ler PART_ID (registrador 0xFF)
    uint8_t reg_addr = 0xFF;
    uint8_t part_id;

    tw_master_transmit(max30102_addr, &reg_addr, 1, true);
    tw_master_receive(max30102_addr, &part_id, 1);

    printf("PART_ID: 0x%02X\n", part_id);

    while(1) {
        _delay_ms(1000);
    }

    return 0;
}