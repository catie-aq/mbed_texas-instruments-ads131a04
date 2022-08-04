/*
 * Copyright (c) 2021, CATIE, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CATIE_SIXTRON_ADS131A04_H_
#define CATIE_SIXTRON_ADS131A04_H_

#include "mbed.h"

namespace sixtron {

#define CHIP_ID 0x04
#define WORD_LENGTH (MBED_CONF_ADS131A04_WORD_LENGTH >> 3)

#define STATUS_INIT 0xFF04
#define STATUS_OK 0x0655

#define CLKIN_DIV_2 1
#define CLKIN_DIV_4 2
#define CLKIN_DIV_6 3
#define CLKIN_DIV_8 4
#define CLKIN_DIV_10 5
#define CLKIN_DIV_12 6
#define CLKIN_DIV_14 7

#define FICLK_DIV_2 1
#define FICLK_DIV_4 2
#define FICLK_DIV_6 3
#define FICLK_DIV_8 4
#define FICLK_DIV_10 5
#define FICLK_DIV_12 6
#define FICLK_DIV_14 7

#define OSR_4096 0x00
#define OSR_2048 0x01
#define OSR_1024 0x02
#define OSR_800 0x03
#define OSR_768 0x04
#define OSR_512 0x05
#define OSR_400 0x06
#define OSR_384 0x07
#define OSR_256 0x08
#define OSR_200 0x09
#define OSR_192 0x0A
#define OSR_128 0x0B
#define OSR_96 0x0C
#define OSR_64 0x0D
#define OSR_48 0x0E
#define OSR_32 0x0F

typedef struct adc_data_struct {
    uint16_t response;
    int32_t channel1;
    int32_t channel2;
    int32_t channel3;
    int32_t channel4;
} adc_data_struct;

class ADS131A04 {
public:
    enum class ADC : uint8_t {
        all = 0x00,
        adc1 = 0x01,
        adc2 = 0x02,
        adc3 = 0x03,
        adc4 = 0x04,
    };

    enum class Frequency : uint8_t { _2500Hz, _2000Hz, _1000Hz, _500Hz, _250Hz };

    ADS131A04(SPI *spi, PinName cs, PinName reset, PinName drdy);

    int8_t init();

    int8_t set_gain(ADC adc, uint8_t gain);

    int8_t set_frequency(uint8_t clkin_div, uint8_t ficlk_div, uint8_t fmod_div);
    int8_t set_frequency(Frequency freq);

    int8_t start();

    int8_t stop();

    int8_t read_adc_data(adc_data_struct *dataStruct);

    void attach_callback(Callback<void()> func);

private:
    enum class Command : uint16_t {
        // system command
        null = 0x0000,
        reset = 0x0011,
        standby = 0x0022,
        wakeup = 0x0033,
        lock = 0x555,
        unlock = 0x655,

        // register write ans read commands
        rreg = 0x20,
        rregs = 0x20,
        wreg = 0x40,
        wregs = 0x60
    };

    enum class RegisterAddress : uint8_t {
        // Read only register
        id_msb = 0x00,
        id_lsb = 0x01,

        // Status Registers
        stat_1 = 0x02,
        stat_p = 0x03,
        stat_n = 0x04,
        stat_s = 0x05,
        error_cnt = 0x06,
        stat_m2 = 0x07,

        // User Configuration Registers
        a_sys_cfg = 0x0B,
        d_sys_cfg = 0x0C,
        clk1 = 0x0D,
        clk2 = 0x0E,
        adc_ena = 0x0F,
        adc1 = 0x11,
        adc2 = 0x12,
        adc3 = 0x13,
        adc4 = 0x14
    };

    void reset();

    int8_t spi_read_register(RegisterAddress registerAddress, uint8_t *value);

    int8_t spi_write_register(RegisterAddress registerAddress, uint8_t value);

    int8_t send_command(Command command, uint16_t *value = nullptr);

    SPI *_spi;

    DigitalOut _cs;
    DigitalOut _reset;
    InterruptIn _drdy;
};

} // namespace sixtron

#endif // CATIE_SIXTRON_ADS131A04_H_
