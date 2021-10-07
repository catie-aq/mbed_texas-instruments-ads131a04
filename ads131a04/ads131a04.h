/*
 * Copyright (c) 2021, CATIE, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CATIE_SIXTRON_ADS131A04_H_
#define CATIE_SIXTRON_ADS131A04_H_

namespace sixtron {

#define CHIP_ID     0x04

class ADS131A04
{
public:
    enum class Command : char {
        NULL        = 0x0000,
        RESET       = 0x0011,
        STANDBY     = 0x0022,
        WAKEUP      = 0x0033,
        LOCK        = 0x555,
        UNLOCK      = 0x655
    }

    enum class RegisterAddress : char {
        // Read only register
        ID_MSB      = 0x00,
        ID_LSB      = 0x01,

        // Status Registers
        STAT_1      = 0x02,
        STAT_P      = 0x03,
        STAT_N      = 0x04,
        STAT_S      = 0x05,
        ERROR_CNT   = 0x06,
        STAT_M2     = 0x07,

        // User Configuration Registers
        A_SYS_CFG   = 0x0B,
        D_SYS_CFG   = 0x0C,
        CLK1        = 0x0D,
        CLK2        = 0x0E,
        ADC_ENA     = 0x0F,
        ADC1        = 0x11,
        ADC2        = 0x12,
        ADC3        = 0x13,
        ADC4        = 0x14
    }

    ADS131A04();

    uint8_t init();

private:
    void reset();

    uint8_t spi_read_register(RegisterAddress registerAddress, uint8_t *value);

    uint8_t spi_write_register(RegisterAddress registerAddress, uint8_t value);

    uint8_t send_command(Command command, uint16_t *value=nullptr);

private:
    InterrupIn &drdy;
};

} // namespace sixtron

#endif // CATIE_SIXTRON_ADS131A04_H_

