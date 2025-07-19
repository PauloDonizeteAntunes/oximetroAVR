#include "lib/funsape/funsapeLibGlobalDefines.hpp"

typedef union {
    struct {
    };
    uint8_t allFlags;
} systemFlags_t;


int main()
{

    //sei();

    while(true) {

    }

    return 0;
}

void pcint0InterruptCallback(void)
{
    static uint8_t lastState = 0xFF;
    uint8_t currentState = PINB & 0x0F;
    uint8_t changed = lastState ^ currentState;

    if(isBitClr(PINB, PB3) && changed & (1 << PB3)) {
        systemFlags.incUni = !systemFlags.incUni;
    }
    if(isBitClr(PINB, PB2) && changed & (1 << PB2)) {
        systemFlags.incDez = !systemFlags.incDez;
    }
    if(isBitClr(PINB, PB1) && changed & (1 << PB1)) {
        systemFlags.incCen = !systemFlags.incCen;
    }
    if(isBitClr(PINB, PB0) && changed & (1 << PB0)) {
        systemFlags.incMil = !systemFlags.incMil;
    }

    lastState = currentState;

}

void timer0CompareACallback(void)
{
    display.showNextDigit();
}
