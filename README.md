# Projeto AVR para ATmega328p com Sensores e Display

Este projeto é focado na programação do microcontrolador ATmega328P utilizando a arquitetura AVR. O objetivo principal é integrar periféricos essenciais como um display ST7735 e um sensor de oximetria MAX30102, demonstrando a comunicação via SPI e I2C.

## Hardware Utilizado
*   Microcontrolador: **ATmega328P**
*   Display: **ST7735** (comunicação via SPI)
*   Sensor: **MAX30102** (comunicação via I2C)

## Estrutura e Funcionalidades
*   **Inicialização de Periféricos:** Configuração e inicialização dos barramentos SPI e I2C.
*   **Comunicação com Display:** Envio de comandos e dados para o display ST7735 para exibir informações.
*   **Leitura de Sensor:** Integração com o sensor MAX30102 para obter valores de seus registradores internos.

## Bibliotecas e Dependências

Este projeto utiliza um conjunto de bibliotecas para interagir com os periféricos.

| Biblioteca | Link | Descrição |
| :--- | :--- | :--- |
| **ST7735_AVR** | [anothermist/DISPLAYS](https://github.com/anothermist/DISPLAYS/tree/master/ST7735_AVR) | Biblioteca para comunicação via SPI com o display ST7735. |
| **avr-i2c-library** | [Sovichea/avr-i2c-library](https://github.com/Sovichea/avr-i2c-library) | Implementação de baixo nível do protocolo I2C para comunicação com sensores e outros CIs. |
| **funsapeAvr8Lib** | [leandroschwarz/funsapeAvr8Lib](https://github.com/leandroschwarz/funsapeAvr8Lib/tree/development) | Biblioteca auxiliar para inicialização e abstração de periféricos do ATmega328P. |
| **SparkFun MAX3010x**| [sparkfun/SparkFun_MAX3010x](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library) | Utilizada parcialmente para consultar os endereços e valores dos registradores do sensor MAX30102. O código não foi importado por completo. |

## Como Compilar e Usar
1.  Certifique-se de que o AVR-GCC e o `avrdude` estão instalados no seu sistema.
2.  Clone este repositório.
3.  Ajuste as configurações de pinos no código-fonte, se necessário, para corresponder ao seu hardware.
4.  Compile o projeto utilizando o `Makefile` fornecido ou sua ferramenta de preferência.
5.  Faça o upload do firmware para o ATmega328P usando um programador (ex: USBasp).

## Licença


***

```
MIT License

Copyright (c) 2025 [Seu Nome ou Nome de Usuário do GitHub]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
