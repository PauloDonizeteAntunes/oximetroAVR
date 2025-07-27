#include "../lib/funsape/funsapeLibGlobalDefines.hpp"
#include "../lib/funsape/peripheral/funsapeLibUsart0.hpp"
#include "../lib/funsape/peripheral/funsapeLibInt0.hpp"
#include "../lib/MAX30102.h"
#include "../lib/twi_master.h"
#include "../lib/calcMaster.h"

volatile bool fifo_rdy = 0;
volatile bool bpm_rdy = 0;

uint32_t tendeciaIr[3];
uint8_t controle = 16;
uint8_t x = 0;

uint32_t bpmAmostra[MAX_VALES];
uint16_t bpmIndex = 0;

const uint32_t tamanho = sizeof(bpmAmostra) / sizeof(bpmAmostra[0]);
uint16_t indices_vales[MAX_VALES];


void usartConfg(){
    usart0.setBaudRate(Usart0::BaudRate::BAUD_RATE_57600);
    usart0.init();               // primeiro init
    usart0.enableTransmitter();  // depois habilita TX
    usart0.stdio();              // só agora vincula // printf() ao usart0
}

void processarLeituras(void) {

        uint8_t available = getAvailableSamples();

        if (available > 0) {
            for (uint8_t i = 0; i < available; i++) {
                uint32_t red, ir;
                uint32_t irMedia = 0;

                for (uint8_t j = 0; j < controle; j++) {
                    readFIFO(&red, &ir);
                    irMedia += ir;
                }

                irMedia /= controle;
                tendeciaIr[x++] = irMedia;

                if (x == 3) {
                    x = 0;
                    uint32_t retorno1 = calcularTendencia(tendeciaIr);

                    for (uint8_t k = 0; k < 3; k++)
                        tendeciaIr[k] = 0;

                    if (retorno1 > 5000) {
                        bpmAmostra[bpmIndex++] = retorno1;
                    }

                    if (bpmIndex >= tamanho) {
                        bpmIndex = 0;
                        float bpm = detectarValesEBPM(
                            bpmAmostra, tamanho, indices_vales, 10);
                        uint16_t parte_int = (uint16_t)bpm;
                        uint16_t parte_dec = (uint16_t)((bpm - parte_int) * 100);
                        printf("BPM estimado: %u.%02u\n\r", parte_int, parte_dec);
                    }
                }
            }

            readRegister(MAX30102_INT_STATUS_1);
            readRegister(MAX30102_INT_STATUS_2);
            int0.clearInterruptRequest();
            fifo_rdy = false;
        }
}



int main(void)
{

    usartConfg();

    printf("COMECOU\n\r");

    if (!initMAX30102()) {
        // printf("Failed to initialize MAX30102\n");
        return -1;
    }

    sei();

    while (1) {
        if(fifo_rdy){
            processarLeituras();
        }
    }

    return 0;
}

void int0InterruptCallback(void)
{
    fifo_rdy = true;
}