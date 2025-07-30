//!
//! \file           main.cpp
//! \brief          main code of a Heart Rate and Pulse Oximetry Sensor integrated with a TFT display
//! \author         Paulo Donizete Antunes Junior
//! \date           2025-07-30
//! \version        1.0
//! \details        main code of a Heart Rate and Pulse Oximetry Sensor integrated with a TFT display
//!


#include "../lib/funsape/funsapeLibGlobalDefines.hpp"
#include "../lib/funsape/peripheral/funsapeLibUsart0.hpp"
#include "../lib/funsape/peripheral/funsapeLibInt0.hpp"
#include "../lib/funsape/peripheral/funsapeLibInt1.hpp"
#include "../lib/funsape/peripheral/funsapeLibTimer0.hpp"
#include "../lib/MAX30102/MAX30102.h"
#include "../lib/TWI/twi_master.h"
#include "../lib/MAX30102/calcMaster.h"
#include "../lib/st7735/st7735.h"
#include "../fonts/Font_8_Retro.h"

// #include "../lib/fatFs/mmc_avr.h"
// #include "../lib/fatFs/ff.h"

//====================================
// Variaveis de controle
//====================================

// var de controle de int
volatile bool fifo_rdy        = 0;
volatile bool bpm_rdy         = 0;
volatile bool debug_rdy       = 0;

// Var de remocao de ruidos e triangulição de ruidos
uint32_t tendeciaIr[3];
uint8_t  controle        = 16;
uint8_t  tendeciaIrIndex = 0;

// var de buffer utilizado para cal do bpm
uint32_t bpmAmostra[MAXVALUES];
uint16_t bpmIndex = 0;
const uint32_t tamanho = sizeof(bpmAmostra) / sizeof(bpmAmostra[0]);
uint16_t indices_vales[MAXVALUES];

// var de buffer utilizado para exibição bpm e checagem em caso de bpm igual.
volatile uint16_t bpm_parte_int = 0;
volatile uint16_t bpm_parte_dec = 0;
volatile uint16_t last_bpm_parte_int = 0;
volatile uint16_t last_bpm_parte_dec = 0;

//buffer usado para escrita na tela
char str[40];

//====================================
// Fim variaveis de controle
//====================================


//====================================
// Funcoes presentes na main
//====================================

void usartConfg(void); // Habilita usart

void processaBPM(volatile uint16_t* parte_int,
                       volatile uint16_t* parte_dec); // Coração da conta do bpm veja função para mais detalhes
void picIfsc(void);   // Exibie a logo do ifsc

//====================================
// Fim das funcoes presentes na main
//====================================

int main(void)
{
    usartConfg();
    sei();

    printf("[01] Programa iniciando ----- \r\n");
    printf("[02] Uart     iniciado  ----- \r\n");
    printf("[03] Interrup habilitadas ----- \r\n");

    //init1 para habilitacao de debug_rdy;
    setBit(PORTD, PD3);
    int1.init(Int1::SenseMode::FALLING_EDGE);
    int1.clearInterruptRequest();
    int1.activateInterrupt();
    printf("[04] INT1     configurado ----- \r\n");

    //Init display tft e SPI
    LCD_Init(2, 3);
    printf("[05] Display  iniciado  ----- \r\n");

    LCD_Rect_Fill(0, 0, 160, 128, BLACK);
    picIfsc(); // Desenha logo do ifsc + o nome do autor
    printf("[06] Tela     inicial    ----- \r\n");

    //init MAX30102 e TWI
    if (!initMAX30102()) {
        printf("-=01=- MAX30102 falhou     ----- \r\n");
        while(1);
    }

    printf("[07] MAX30102 iniciado  ----- \r\n");


    timer0.init(Timer0::Mode::CTC_OCRA, Timer0::ClockSource::PRESCALER_64);
    timer0.setCompareAValue(125); // 0.5ms
    timer0.clearCompareAInterruptRequest();
    timer0.activateCompareAInterrupt();
    printf("[08] TIMER0   iniciado  ----- \r\n");

    //init0 usado pelo MAX30102 para leitura da FIFO interna;
    setBit(PORTD, PD2);
    int0.init(Int0::SenseMode::FALLING_EDGE);
    int0.clearInterruptRequest();
    int0.activateInterrupt();
    printf("[09] INT0     configurado ----- \r\n");


    while (1) {
        if(fifo_rdy){
            processaBPM(&bpm_parte_int, &bpm_parte_dec);
            printf("[10] BPM processado ----- \r\n");

            if(bpm_parte_int != last_bpm_parte_int || bpm_parte_dec != last_bpm_parte_dec){
                printf("[11.1] Antigo bpm defasado ----- \r\n");

                // printf("BPM estimado: %u.%02u\n\r", bpm_parte_int, bpm_parte_dec);
                last_bpm_parte_dec = bpm_parte_dec;
                last_bpm_parte_int = bpm_parte_int;

                bpm_rdy = true;
            }
        }

        if(bpm_rdy == true){

            printf("[12] BPM exibido ----- \r\n");
            snprintf(str, 40, "BPM: %u.%02u\n\r", bpm_parte_int, bpm_parte_dec);
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

void int1InterruptCallback(void){

    static uint8_t lastState = 0xFF;
    uint8_t currentState = PIND & 0x0F;
    uint8_t changed = lastState ^ currentState;

    if(isBitClr(PIND, PD3) && changed & (1 << PD3) ) {
        debug_rdy = true;
        printf("[XX] DEBUG ATIVADO\r\n");
        printf("     debug_rdy [%d]\r\n", debug_rdy);
        picIfsc();
    } else {
        debug_rdy = false;
        picIfsc();
    }

    lastState = currentState;

}

void timer0CompareACallback(void){


}

void processaBPM(volatile uint16_t* parte_int, volatile uint16_t* parte_dec) {

        uint8_t available = getAvailableSamples();

        if (available > 0) {
            fifo_rdy = false;

            for (uint8_t i = 0; i < available; i++) {
                uint32_t red = 0, ir = 0;
                uint32_t irMedia = 0;

                for (uint8_t j = 0; j < controle; j++) {
                    readFIFO(&red, &ir);
                    irMedia += ir;

                }

                if(debug_rdy){
                printf("RED: %lu \t IR: %lu\r\n", red, ir);
                }

                irMedia /= controle;
                tendeciaIr[tendeciaIrIndex++] = irMedia;

                if (tendeciaIrIndex == 3) {
                    tendeciaIrIndex = 0;
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

                        if(debug_rdy == 1){
                            LCD_Orientation(0, 2);
                            LCD_Rect_Fill(65, 71, 52, 32, BLACK);
                            snprintf(str, 40, "RED: %u\n\r", red);
                            LCD_Font(28, 87, str, _8_Retro, 1, BLUE);
                            snprintf(str, 40, "IR:  %u\n\r", ir);
                            LCD_Font(28, 103, str, _8_Retro, 1, WHITE);
                        }

                    }
                }
            }



            readRegister(MAX30102_INT_STATUS_1);
            readRegister(MAX30102_INT_STATUS_2);
            int0.clearInterruptRequest();
        }
}

void usartConfg(void){
    usart0.setBaudRate(Usart0::BaudRate::BAUD_RATE_57600);
    usart0.init();               // primeiro init
    usart0.enableTransmitter();  // depois habilita TX
    usart0.stdio();              // só agora vincula // printf() ao usart0
}

void picIfsc(void){

    uint32_t color;

    if(debug_rdy == 1){
        color = BLUE;

        LCD_Orientation(0, 2);
        snprintf(str, 40, "RED: %u\n\r", 0);
        LCD_Rect_Fill(65, 71, 52, 32, BLACK);
        LCD_Font(28, 87, str, _8_Retro, 1, WHITE);
        snprintf(str, 40, "IR:  %u\n\r", 0);
        LCD_Font(28, 103, str, _8_Retro, 1, WHITE);

    }else{
        color = LIME;

        LCD_Orientation(0, 2);
        LCD_Rect_Fill(28, 71, 52, 32, BLACK);
    }

    // Bloco usado para desenhar simbolo do ifsc ---------------------------------

    LCD_Orientation(0, 3);

    LCD_Circle(8, 118, 4, 1, 1, RED);

    //Linha 1
    LCD_Rect_Fill(4, 101, 9, 9, color);
    LCD_Rect_Fill(4, 87, 9, 9, color);

    //Linha 2
    LCD_Rect_Fill(15, 114, 9, 9, color);
    LCD_Rect_Fill(15, 101, 9, 9, color);

    //Linha 3
    LCD_Rect_Fill(27, 114, 9, 9, color);
    LCD_Rect_Fill(27, 101, 9, 9, color);
    LCD_Rect_Fill(27,  87, 9, 9, color);

    //Linha 4
    LCD_Rect_Fill(39, 114, 9, 9, color);
    LCD_Rect_Fill(39, 101, 9, 9, color);

    //---------------------------------------------------------------------------

    LCD_Orientation(0, 2);

    // Escola + autor
    LCD_Font(48, 18, "-=IFSC=-", _8_Retro, 1, WHITE);
    LCD_Font(48, 36, "Paulo", _8_Retro, 1, WHITE);

    // Inicializa bpm na tela
    snprintf(str, 40, "BPM: %u.%02u\n\r", 0, 0);
    LCD_Rect_Fill(65, 54, 52, 16, BLACK);
    LCD_Font(28, 70, str, _8_Retro, 1, WHITE);

}

