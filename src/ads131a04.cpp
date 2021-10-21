/*
 * Copyright (c) 2021, CATIE, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ads131a04/ads131a04.h"

namespace sixtron {

ADS131A04::ADS131A04(SPI *spi, PinName cs, PinName reset): _spi(spi)
{
    _cs = new DigitalOut(cs);

    _reset = new DigitalOut(reset);

    _spi->format(8, 1);
}

void ADS131A04::reset()
{
    // TODO : Implement
}

uint8_t ADS131A04::init()
{
    uint16_t status = 0;

    send_command(Command::null, &status);

    if (status == 0xff04) {
        status = send_command(Command::unlock);

        if (status == 0x0655) {
            spi_write_register(RegisterAddress::clk1, 0x02); // CLKIN/2

            spi_write_register(RegisterAddress::clk2, 0x25); // FMOD/512 | ICLK/2

            // Enable All ADC channel
            spi_write_register(RegisterAddress::adc_ena, 0x0F);

            send_command(Command::lock);
        }
    }

    return 0;
}

uint8_t ADS131A04::start()
{
    uint16_t status = 0;

    status = send_command(Command::unlock);

    if (status == 0x0655) {
        // Enable All ADC channel
        spi_write_register(RegisterAddress::adc_ena, 0x0F);

        // Wake-up from standby mode
        send_command(Command::wakeup);

        send_command(Command::lock);

        return 0;
    }

    return -1;
}

uint8_t ADS131A04::stop()
{
    uint16_t status = 0;

    status = send_command(Command::unlock);

    if (status == 0x0655) {
        // Enable All ADC channel
        spi_write_register(RegisterAddress::adc_ena, 0x00);

        // Wake-up from standby mode
        send_command(Command::wakeup);

        send_command(Command::lock);

        return 0;
    }

    return -1;
}

uint8_t ADS131A04::set_gain(ADC adc, uint8_t gain)
{
    switch (adc) {
        case ADC::adc1:
            spi_write_register(RegisterAddress::adc1, gain);
            break;
        case ADC::adc2:
            spi_write_register(RegisterAddress::adc2, gain);
            break;
        case ADC::adc3:
            spi_write_register(RegisterAddress::adc3, gain);
            break;
        case ADC::adc4:
            spi_write_register(RegisterAddress::adc4, gain);
            break;
        case ADC::all:
            spi_write_register(RegisterAddress::adc1, gain);
            spi_write_register(RegisterAddress::adc2, gain);
            spi_write_register(RegisterAddress::adc3, gain);
            spi_write_register(RegisterAddress::adc4, gain);
            break;
        default:
            return -1;
    }

    return 0;
}

uint8_t ADS131A04::spi_read_register(RegisterAddress registerAddress, uint8_t *value)
{
    uint16_t word
            = static_cast<uint8_t>(Command::rreg) | (static_cast<uint8_t>(registerAddress) << 8);

    char txData[2] = { 0 };
    txData[1] = (word & 0xff);
    txData[0] = (word >> 8);

    char ret[3]; // = 0;

    *_cs = 0;

    _spi->write(txData, 2, ret, 3);

    *_cs = 1;

    for (int i = 0; i < 3; i++) {
        printf("%d\n", ret[i]);
    }

    printf("Read register %d : %d\n", (uint8_t)registerAddress, ret);

    return 1;
}

uint8_t ADS131A04::spi_write_register(RegisterAddress registerAddress, uint8_t value)
{
    uint16_t word
            = static_cast<uint8_t>(Command::wreg) | static_cast<uint8_t>(registerAddress) >> 8;

    *_cs = 0;

    _spi->write((const char *)&word, 2, nullptr, 0);

    *_cs = 1;

    return 0;
}

uint8_t ADS131A04::send_command(Command command, uint16_t *value)
{
    uint16_t word = static_cast<uint16_t>(Command::rreg);

    *_cs = 0;

    _spi->write((const char *)&word, 2, (char *)value, 2);

    *_cs = 1;

    return 0;
}

} // namespace sixtron
