#include "../lib/funsape/funsapeLibGlobalDefines.hpp"
#include "../lib/funsape/peripheral/funsapeLibUsart0.hpp"
#include "../lib/funsape/peripheral/funsapeLibInt0.hpp"
#include "../lib/MAX30102.h"
#include "../lib/twi_master.h"
#include "../lib/calcMaster.h"
#include "../lib/main_.h"
#include "../lib/st7735.h"
#include "../fonts/Font_8_Retro.h"

#include "../lib/fatFs/mmc_avr.h"
#include "../lib/fatFs/ff.h"


volatile bool fifo_rdy = 0;
volatile bool bpm_rdy = 0;
volatile uint16_t bpm_parte_int = 0;
volatile uint16_t bpm_parte_dec = 0;

volatile uint16_t last_bpm_parte_int = 0;
volatile uint16_t last_bpm_parte_dec = 0;


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

void processarLeituras(volatile uint16_t* parte_int, volatile uint16_t* parte_dec) {

        uint8_t available = getAvailableSamples();

        if (available > 0) {
            fifo_rdy = false;

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
                        *parte_int = (uint16_t)bpm;
                        *parte_dec = (uint16_t)((bpm - *parte_int) * 100);
                        // printf("BPM estimado: %u.%02u\n\r", parte_int, parte_dec);
                    }
                }
            }

            readRegister(MAX30102_INT_STATUS_1);
            readRegister(MAX30102_INT_STATUS_2);
            int0.clearInterruptRequest();
        }
}



int main(void)
{

    usartConfg();

    printf("COMECOU\n\r");

    sei();

    LCD_Init(2, 3);
    // TESTE BÁSICO - Apenas uma cor sólida primeiro
    LCD_Rect_Fill(0, 0, 160, 128, BLACK);
    // _delay_ms(2000);

    //IFSC moment
    LCD_Circle(8, 118, 4, 1, 1, RED);

    //Linha 1
    LCD_Rect_Fill(4, 101, 9, 9, LIME);
    LCD_Rect_Fill(4, 87, 9, 9, LIME);

    //Linha 2
    LCD_Rect_Fill(15, 114, 9, 9, LIME);
    LCD_Rect_Fill(15, 101, 9, 9, LIME);

    //Linha 3
    LCD_Rect_Fill(27, 114, 9, 9, LIME);
    LCD_Rect_Fill(27, 101, 9, 9, LIME);
    LCD_Rect_Fill(27,  87, 9, 9, LIME);

    //Linha 4
    LCD_Rect_Fill(39, 114, 9, 9, LIME);
    LCD_Rect_Fill(39, 101, 9, 9, LIME);

    LCD_Orientation(0, 2);
    LCD_Font(48, 18, "-=IFSC=-", _8_Retro, 1, WHITE);
    LCD_Font(48, 36, "Paulo", _8_Retro, 1, WHITE);
    // _delay_ms(1000);

    char str[30];
    snprintf(str, 30, "BPM: %u.%02u\n\r", 0, 0);
    LCD_Rect_Fill(65, 54, 52, 16, BLACK);
    LCD_Font(28, 70, str, _8_Retro, 1, WHITE);


    // LCD_Orientation(0, 0);

    if (!initMAX30102()) {
        // printf("Failed to initialize MAX30102\n");
        return -1;
    }


    while (1) {
        if(fifo_rdy){
            processarLeituras(&bpm_parte_int, &bpm_parte_dec);

            if(bpm_parte_int != last_bpm_parte_int || bpm_parte_dec != last_bpm_parte_dec){
                // printf("BPM estimado: %u.%02u\n\r", bpm_parte_int, bpm_parte_dec);
                last_bpm_parte_dec = bpm_parte_dec;
                last_bpm_parte_int = bpm_parte_int;

                bpm_rdy = true;
            }

        }

        if(bpm_rdy == true){
            snprintf(str, 30, "BPM: %u.%02u\n\r", bpm_parte_int, bpm_parte_dec);
            LCD_Rect_Fill(65, 54, 52, 16, BLACK);
            LCD_Font(28, 70, str, _8_Retro, 1, WHITE);

            bpm_rdy = false;
        }


    }

    return 0;
}

void int0InterruptCallback(void)
{
    fifo_rdy = true;
}