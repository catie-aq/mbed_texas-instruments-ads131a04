/*
 * Copyright (c) 2021, CATIE, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CATIE_SIXTRON_ADS131A04_H_
#define CATIE_SIXTRON_ADS131A04_H_

#include "mbed.h"

namespace sixtron {

#define CHIP_ID 0x04
#define WORD_LENGTH 4

class ADS131A04 {
public:
    enum class ADC : uint8_t { all = 0x00, adc1 = 0x01, adc2 = 0x02, adc3 = 0x03, adc4 = 0x04 };

    ADS131A04(SPI *spi, PinName cs, PinName reset);

    uint8_t init();

    uint8_t set_gain(ADC adc, uint8_t gain);

    uint8_t start();

    uint8_t stop();

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
        rreg = 0x2000,
        rregs = 0x2000,
        wreg = 0x4000,
        wregs = 0x6000
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

    uint8_t unlock();

    uint8_t lock();

    uint8_t spi_read_register(RegisterAddress registerAddress, uint8_t *value);

    uint8_t spi_write_register(RegisterAddress registerAddress, uint8_t value);

    uint8_t send_command(Command command, uint16_t *value = nullptr);

    SPI *_spi;

    DigitalOut *_cs;
    DigitalOut *_reset;

    // InterruptIn &drdy;
};

} // namespace sixtron

#endif // CATIE_SIXTRON_ADS131A04_H_
