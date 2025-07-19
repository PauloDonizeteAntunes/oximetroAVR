#include "../lib/funsape/funsapeLibGlobalDefines.hpp"
#include "../lib/funsape/peripheral/funsapeLibUsart0.hpp"

typedef union {
    struct {
    };
    uint8_t allFlags;
} systemFlags_t;


int main()
{

    usart0.setBaudRate(Usart0::BaudRate::BAUD_RATE_9600);
    usart0.init();

    usart0.stdio();

    sei();

    while(true) {

    printf("teste");
    delayMs(1000);

    }

    return 0;
}
