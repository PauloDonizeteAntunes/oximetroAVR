//!
//! \file           max30102.hpp
//! \brief          MAX30102 Heart Rate and Pulse Oximetry Sensor Library
//! \author         Generated for ATmega328P
//! \date           2025-07-20
//! \version        1.0
//! \details        Biblioteca bare metal para controle do sensor MAX30102
//!

#ifndef MAX30102_HPP
#define MAX30102_HPP

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Definições de registradores do MAX30102
// =============================================================================

// I2C Address
#define MAX30102_I2C_ADDRESS        0x57

// Status Registers
#define MAX30102_INT_STATUS_1       0x00
#define MAX30102_INT_STATUS_2       0x01
#define MAX30102_INT_ENABLE_1       0x02
#define MAX30102_INT_ENABLE_2       0x03

// FIFO Registers
#define MAX30102_FIFO_WR_PTR        0x04
#define MAX30102_OVF_COUNTER        0x05
#define MAX30102_FIFO_RD_PTR        0x06
#define MAX30102_FIFO_DATA          0x07

// Configuration Registers
#define MAX30102_FIFO_CONFIG        0x08
#define MAX30102_MODE_CONFIG        0x09
#define MAX30102_SPO2_CONFIG        0x0A
#define MAX30102_LED1_PA            0x0C  // Red LED
#define MAX30102_LED2_PA            0x0D  // IR LED
#define MAX30102_PILOT_PA           0x10
#define MAX30102_MULTI_LED_CTRL1    0x11
#define MAX30102_MULTI_LED_CTRL2    0x12

// Die Temperature Registers
#define MAX30102_DIE_TEMP_INT       0x1F
#define MAX30102_DIE_TEMP_FRAC      0x20
#define MAX30102_DIE_TEMP_CONFIG    0x21

// Proximity Function Registers
#define MAX30102_PROX_INT_THRESH    0x30

// Part ID Registers
#define MAX30102_REV_ID             0xFE
#define MAX30102_PART_ID            0xFF

// =============================================================================
// Máscaras de bits e constantes
// =============================================================================

// Interrupt Status/Enable Bits
#define MAX30102_INT_A_FULL         (1 << 7)
#define MAX30102_INT_DATA_RDY       (1 << 6)
#define MAX30102_INT_ALC_OVF        (1 << 5)
#define MAX30102_INT_PROX_INT       (1 << 4)
#define MAX30102_INT_PWR_RDY        (1 << 0)

#define MAX30102_INT_DIE_TEMP_RDY   (1 << 1)

// Mode Configuration
#define MAX30102_MODE_SHDN          (1 << 7)
#define MAX30102_MODE_RESET         (1 << 6)
#define MAX30102_MODE_HEART_RATE    0x02
#define MAX30102_MODE_SPO2          0x03
#define MAX30102_MODE_MULTI_LED     0x07

// SpO2 Configuration
#define MAX30102_SPO2_ADC_RGE_2048  0x00
#define MAX30102_SPO2_ADC_RGE_4096  0x20
#define MAX30102_SPO2_ADC_RGE_8192  0x40
#define MAX30102_SPO2_ADC_RGE_16384 0x60

#define MAX30102_SPO2_SR_50         0x00
#define MAX30102_SPO2_SR_100        0x04
#define MAX30102_SPO2_SR_200        0x08
#define MAX30102_SPO2_SR_400        0x0C
#define MAX30102_SPO2_SR_800        0x10
#define MAX30102_SPO2_SR_1000       0x14
#define MAX30102_SPO2_SR_1600       0x18
#define MAX30102_SPO2_SR_3200       0x1C

#define MAX30102_SPO2_PW_69         0x00
#define MAX30102_SPO2_PW_118        0x01
#define MAX30102_SPO2_PW_215        0x02
#define MAX30102_SPO2_PW_411        0x03

// FIFO Configuration
#define MAX30102_SAMPLEAVG_1        0x00
#define MAX30102_SAMPLEAVG_2        0x20
#define MAX30102_SAMPLEAVG_4        0x40
#define MAX30102_SAMPLEAVG_8        0x60
#define MAX30102_SAMPLEAVG_16       0x80
#define MAX30102_SAMPLEAVG_32       0xA0

#define MAX30102_ROLLOVER_EN        0x10
#define MAX30102_A_FULL_MASK        0x0F

// Constantes
#define MAX30102_FIFO_SIZE          32
#define MAX30102_PART_ID_VALUE      0x15

bool initMAX30102();
void readFIFO(uint32_t* red, uint32_t* ir);
uint8_t getAvailableSamples();

//Twi function
uint8_t readRegister(uint8_t reg);
void writeRegister(uint8_t reg, uint8_t value);


#endif // MAX30102_HPP