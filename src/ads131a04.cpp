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
        // Disable All ADC channel
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
    static char data[2];
    data[0] = static_cast<char>(Command::rreg) | static_cast<char>(registerAddress);
    data[1] = 0x00;

    *_cs = 0;

    if (_spi->write(data, 2, nullptr, 0) != 0) {
        return -1;
    }

    char ret = 0;

    if (_spi->write(&ret, 1, &ret, 1) != 0) {
        return -1;
    }

    *_cs = 1;

    return ret;
}

uint8_t ADS131A04::spi_write_register(RegisterAddress registerAddress, uint8_t value)
{
    static char data[2];
    data[0] = static_cast<char>(Command::wreg) | static_cast<char>(registerAddress);
    data[1] = static_cast<char>(value);

    *_cs = 0;

    if (_spi->write(data, 2, nullptr, 0) != 0) {
        return -1;
    }

    char ret = 0;

    if (_spi->write(&ret, 1, &ret, 1) != 0) {
        return -1;
    }

    *_cs = 1;

    return ret;
}

uint8_t ADS131A04::send_command(Command command, uint16_t *value)
{
    static char data[2];
    data[0] = static_cast<char>(Command::wreg);
    data[1] = 0x00;

    *_cs = 0;

    if (_spi->write(data, 2, data, 2) != 0) {
        return -1;
    }

    *_cs = 1;

    *value = ((uint16_t)data[0] << 8) | ((uint16_t)data[1] & 0x00FF);

    return 0;
}

} // namespace sixtron
