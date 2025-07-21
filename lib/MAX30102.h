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

// =============================================================================
// Estruturas e tipos de dados
// =============================================================================

typedef enum {
    MAX30102_MODE_HEART_RATE_ONLY = MAX30102_MODE_HEART_RATE,
    MAX30102_MODE_SPO2_ONLY = MAX30102_MODE_SPO2,
    MAX30102_MODE_MULTI_LED_MODE = MAX30102_MODE_MULTI_LED
} max30102_mode_t;

typedef enum {
    MAX30102_SAMPLE_RATE_50 = MAX30102_SPO2_SR_50,
    MAX30102_SAMPLE_RATE_100 = MAX30102_SPO2_SR_100,
    MAX30102_SAMPLE_RATE_200 = MAX30102_SPO2_SR_200,
    MAX30102_SAMPLE_RATE_400 = MAX30102_SPO2_SR_400,
    MAX30102_SAMPLE_RATE_800 = MAX30102_SPO2_SR_800,
    MAX30102_SAMPLE_RATE_1000 = MAX30102_SPO2_SR_1000,
    MAX30102_SAMPLE_RATE_1600 = MAX30102_SPO2_SR_1600,
    MAX30102_SAMPLE_RATE_3200 = MAX30102_SPO2_SR_3200
} max30102_sample_rate_t;

typedef enum {
    MAX30102_PULSE_WIDTH_69 = MAX30102_SPO2_PW_69,
    MAX30102_PULSE_WIDTH_118 = MAX30102_SPO2_PW_118,
    MAX30102_PULSE_WIDTH_215 = MAX30102_SPO2_PW_215,
    MAX30102_PULSE_WIDTH_411 = MAX30102_SPO2_PW_411
} max30102_pulse_width_t;

typedef enum {
    MAX30102_ADC_RANGE_2048 = MAX30102_SPO2_ADC_RGE_2048,
    MAX30102_ADC_RANGE_4096 = MAX30102_SPO2_ADC_RGE_4096,
    MAX30102_ADC_RANGE_8192 = MAX30102_SPO2_ADC_RGE_8192,
    MAX30102_ADC_RANGE_16384 = MAX30102_SPO2_ADC_RGE_16384
} max30102_adc_range_t;

typedef enum {
    MAX30102_SAMPLE_AVG_1 = MAX30102_SAMPLEAVG_1,
    MAX30102_SAMPLE_AVG_2 = MAX30102_SAMPLEAVG_2,
    MAX30102_SAMPLE_AVG_4 = MAX30102_SAMPLEAVG_4,
    MAX30102_SAMPLE_AVG_8 = MAX30102_SAMPLEAVG_8,
    MAX30102_SAMPLE_AVG_16 = MAX30102_SAMPLEAVG_16,
    MAX30102_SAMPLE_AVG_32 = MAX30102_SAMPLEAVG_32
} max30102_sample_avg_t;

typedef struct {
    uint32_t red;
    uint32_t ir;
    uint32_t green;
} max30102_sample_t;

// =============================================================================
// Classe MAX30102
// =============================================================================

class MAX30102 {
private:
    bool initialized;
    max30102_mode_t current_mode;
    uint8_t led_power_red;
    uint8_t led_power_ir;

    // Métodos privados para I2C
    bool i2c_write_register(uint8_t reg, uint8_t value);
    bool i2c_read_register(uint8_t reg, uint8_t* value);
    bool i2c_read_multiple(uint8_t reg, uint8_t* buffer, uint8_t length);

    // Métodos auxiliares
    void clear_fifo();

public:
    // Construtor/Destrutor
    MAX30102();
    ~MAX30102();

    // Inicialização e configuração
    bool begin();
    bool reset();
    bool check_part_id();

    // Configuração de modo
    bool set_mode(max30102_mode_t mode);
    bool shutdown();
    bool wakeup();

    // Configuração de LEDs
    bool set_led_power(uint8_t red_power, uint8_t ir_power);
    bool set_red_led_power(uint8_t power);
    bool set_ir_led_power(uint8_t power);

    // Configuração do ADC e amostragem
    bool set_adc_range(max30102_adc_range_t range);
    bool set_sample_rate(max30102_sample_rate_t rate);
    bool set_pulse_width(max30102_pulse_width_t width);
    bool set_sample_averaging(max30102_sample_avg_t samples);

    // Configuração de FIFO
    bool set_fifo_config(max30102_sample_avg_t sample_avg, bool rollover, uint8_t almost_full);

    // Configuração de interrupções
    bool enable_data_ready_interrupt();
    bool disable_data_ready_interrupt();
    bool enable_fifo_almost_full_interrupt();
    bool disable_fifo_almost_full_interrupt();

    // Leitura de dados
    bool data_available();
    uint8_t get_available_samples();
    bool read_sample(max30102_sample_t* sample);
    bool read_fifo_samples(max30102_sample_t* samples, uint8_t max_samples, uint8_t* samples_read);

    // Temperatura
    bool read_temperature(float* temperature);

    // Status e diagnóstico
    bool get_interrupt_status(uint8_t* status1, uint8_t* status2);
    bool clear_interrupts();

    // Utilitários
    uint8_t get_fifo_write_pointer();
    uint8_t get_fifo_read_pointer();

    // Configuração rápida para aplicações comuns
    bool setup_for_heart_rate_monitoring();
    bool setup_for_spo2_monitoring();
};

// =============================================================================
// Variável global da instância
// =============================================================================

extern MAX30102 max30102;

#endif // MAX30102_HPP