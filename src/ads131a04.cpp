/*
 * Copyright (c) 2021, CATIE, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ads131a04/ads131a04.h"

namespace sixtron {

ADS131A04::ADS131A04(SPI *spi, PinName cs, PinName reset, PinName drdy):
        _spi(spi), _cs(cs), _reset(reset), _drdy(drdy)
{
    _spi->format(8, 1);
}

void ADS131A04::reset()
{
    _reset = 0;
    ThisThread::sleep_for(500ms);
    _reset = 1;
    ThisThread::sleep_for(5ms);

    while (_drdy == 0);

    while (_drdy == 1);

    ThisThread::sleep_for(500ms);
}

int8_t ADS131A04::init()
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

int8_t ADS131A04::start()
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

int8_t ADS131A04::stop()
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

int8_t ADS131A04::set_gain(ADC adc, uint8_t gain)
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

void ADS131A04::set_adc_data_callback(Callback<void()> func)
{
    if (func) {
        _drdy.fall(func);
        _drdy.enable_irq();
    } else {
        _drdy.rise(NULL);
        _drdy.disable_irq();
    }
}

int8_t ADS131A04::read_adc_data(adc_data_struct *adc_data)
{
    static char data[5 * WORD_LENGTH] = { 0 };
    data[0] = 0x00;
    data[1] = 0x00;

    _cs = 0;

    if (_spi->write((char *)data, 2, (char *)data, 5 * WORD_LENGTH) != 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    adc_data->response = ((uint16_t)data[0] << 8) | ((uint16_t)data[1] & 0x00FF);
    adc_data->channel1 = (((int32_t)(((int32_t)data[4] << 24) | ((int32_t)data[5] << 16)
                                  | ((int32_t)data[6] << 8)))
            >> 8);
    adc_data->channel2 = (((int32_t)(((int32_t)data[8] << 24) | ((int32_t)data[9] << 16)
                                  | ((int32_t)data[10] << 8)))
            >> 8);
    adc_data->channel3 = (((int32_t)(((int32_t)data[12] << 24) | ((int32_t)data[13] << 16)
                                  | ((int32_t)data[14] << 8)))
            >> 8);
    adc_data->channel4 = (((int32_t)(((int32_t)data[16] << 24) | ((int32_t)data[17] << 16)
                                  | ((int32_t)data[18] << 8)))
            >> 8);

    return 0;
}

int8_t ADS131A04::spi_read_register(RegisterAddress registerAddress, uint8_t *value)
{
    static char data[WORD_LENGTH] = { 0 };
    data[0] = static_cast<char>(Command::rreg) | static_cast<char>(registerAddress);

    static char ret[WORD_LENGTH] = { 0 };

    _cs = 0;

    if (_spi->write(data, WORD_LENGTH, nullptr, 0) != 0) {
        _cs = 1;
        return -1;
    }

    if (_spi->write(ret, WORD_LENGTH, ret, WORD_LENGTH) != 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    *value = ret[1];

    return 0;
}

int8_t ADS131A04::spi_write_register(RegisterAddress registerAddress, uint8_t value)
{
    static char data[WORD_LENGTH] = { 0 };
    data[0] = static_cast<char>(Command::wreg) | static_cast<char>(registerAddress);
    data[1] = static_cast<char>(value);

    static char ret[WORD_LENGTH] = { 0 };

    _cs = 0;

    if (_spi->write(data, WORD_LENGTH, nullptr, 0) != 0) {
        _cs = 1;
        return -1;
    }

    if (_spi->write(ret, WORD_LENGTH, ret, WORD_LENGTH) != 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    if (ret[0] != data[0]) {
        return -1;
    }

    return 0;
}

int8_t ADS131A04::send_command(Command command, uint16_t *value)
{
    static char data[WORD_LENGTH] = { 0 };
    data[0] = (char)static_cast<uint16_t>(Command::wreg) >> 8;
    data[1] = (char)static_cast<uint16_t>(Command::wreg);

    _cs = 0;

    if (_spi->write(data, WORD_LENGTH, data, WORD_LENGTH) != 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    *value = ((uint16_t)data[0] << 8) | ((uint16_t)data[1] & 0x00FF);

    return 0;
}

} // namespace sixtron
