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
    _spi->frequency(10000000);
    _reset = 1;
    _cs = 1;
}

void ADS131A04::reset()
{
    _reset = 0;
    ThisThread::sleep_for(500ms);
    _reset = 1;
    ThisThread::sleep_for(5ms);
}

int8_t ADS131A04::init()
{
    reset();

    uint16_t status = 0;

    send_command(Command::null, &status);

    if (status == 0xff04) {
        return 0;
    }

    return -1;
}

int8_t ADS131A04::start()
{
    uint16_t status = 0;

    send_command(Command::unlock, &status);

    if (status == 0x0655) {
        // Enable All ADC channel
        spi_write_register(RegisterAddress::adc_ena, 0x0F);

        // Wake-up from standby mode
        send_command(Command::wakeup);

        send_command(Command::lock);

        _drdy.enable_irq();

        return 0;
    }

    return -1;
}

int8_t ADS131A04::stop()
{
    uint16_t status = 0;

    send_command(Command::unlock, &status);

    if (status == 0x0655) {
        // Disable All ADC channel
        spi_write_register(RegisterAddress::adc_ena, 0x00);

        // Standby from Wake-up mode
        send_command(Command::standby);

        send_command(Command::lock);

        _drdy.disable_irq();

        return 0;
    }

    return -1;
}

int8_t ADS131A04::set_gain(ADC adc, uint8_t gain)
{
    uint16_t status = 0;

    send_command(Command::unlock, &status);

    if (status == 0x0655) {
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
        send_command(Command::lock);
    }

    return 0;
}

int8_t ADS131A04::set_frequency(Frequency freq)
{
    uint16_t status = 0;

    send_command(Command::unlock, &status);

    if (status == 0x0655) {
        // Ref CLK: 16.3840 MHz
        switch (freq) {
            case Frequency::_500Hz:
                spi_write_register(RegisterAddress::clk1, 0x08); // CLKIN/8
                spi_write_register(RegisterAddress::clk2, 0x85); // FICLK/8 FMOD/512
                break;
            case Frequency::_1000Hz:
                spi_write_register(RegisterAddress::clk1, 0x08); // CLKIN/8
                spi_write_register(RegisterAddress::clk2, 0x88); // FICLK/8 FMOD/256
                break;
            case Frequency::_2000Hz:
                spi_write_register(RegisterAddress::clk1, 0x08); // CLKIN/8
                spi_write_register(RegisterAddress::clk2, 0x8B); // FICLK/8 FMOD/128
                break;
            case Frequency::_2560Hz:
                spi_write_register(RegisterAddress::clk1, 0x08); // CLKIN/8
                spi_write_register(RegisterAddress::clk2, 0x49); // FICLK/4 FMOD/512
                break;
            case Frequency::_2666Hz:
                spi_write_register(RegisterAddress::clk1, 0x08); // CLKIN/8
                spi_write_register(RegisterAddress::clk2, 0xAD); // FICLK/12 FMOD/64
                break;
            default:
                return -1;
        }
        send_command(Command::lock);
    }

    return 0;
}

void ADS131A04::attach_callback(Callback<void()> func)
{
    if (func) {
        _drdy.fall(func);
    }
}

int8_t ADS131A04::read_adc_data(adc_data_struct *adc_data)
{
    static char data[5 * WORD_LENGTH] = { 0 };
    data[0] = 0x00;
    data[1] = 0x00;

    _cs = 0;

    if (_spi->write((char *)data, 5 * WORD_LENGTH, (char *)data, 5 * WORD_LENGTH) < 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    adc_data->response = ((uint16_t)data[0] << 8) | ((uint16_t)data[1] & 0x00FF);

#if MBED_CONF_ADS131A04_WORD_LENGTH == 32

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
#elif MBED_CONF_ADS131A04_WORD_LENGTH == 24
    adc_data->channel1 = (((int32_t)(((int32_t)data[3] << 24) | ((int32_t)data[4] << 16)
                                  | ((int32_t)data[5] << 8)))
            >> 8);
    adc_data->channel2 = (((int32_t)(((int32_t)data[6] << 24) | ((int32_t)data[7] << 16)
                                  | ((int32_t)data[8] << 8)))
            >> 8);
    adc_data->channel3 = (((int32_t)(((int32_t)data[9] << 24) | ((int32_t)data[10] << 16)
                                  | ((int32_t)data[11] << 8)))
            >> 8);
    adc_data->channel4 = (((int32_t)(((int32_t)data[12] << 24) | ((int32_t)data[13] << 16)
                                  | ((int32_t)data[14] << 8)))
            >> 8);
#else
#error "Word length not supported"
#endif
    return 0;
}

int8_t ADS131A04::spi_read_register(RegisterAddress registerAddress, uint8_t *value)
{
    char data[WORD_LENGTH] = { 0 };
    char receiv[WORD_LENGTH] = { 0 };
    data[0] = static_cast<char>(Command::rreg) | static_cast<char>(registerAddress);

    _cs = 0;

    if (_spi->write(data, WORD_LENGTH, nullptr, 0) < 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    _cs = 0;

    if (_spi->write(receiv, WORD_LENGTH, receiv, WORD_LENGTH) < 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    *value = receiv[1];

    return 0;
}

int8_t ADS131A04::spi_write_register(RegisterAddress registerAddress, uint8_t value)
{
    char data[WORD_LENGTH] = { 0 };
    char receiv[WORD_LENGTH] = { 0 };
    data[0] = static_cast<char>(Command::wreg) | static_cast<char>(registerAddress);
    data[1] = static_cast<char>(value);

    _cs = 0;

    if (_spi->write(data, WORD_LENGTH, nullptr, 0) < 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    _cs = 0;

    if (_spi->write(receiv, WORD_LENGTH, receiv, WORD_LENGTH) < 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    if (receiv[1] != data[1]) {
        return -1;
    }

    return 0;
}

int8_t ADS131A04::send_command(Command command, uint16_t *value)
{
    char receiv[WORD_LENGTH] = { 0 };
    char data[WORD_LENGTH] = { 0 };
    data[0] = (static_cast<uint16_t>(command) >> 8) & 0xff;
    data[1] = static_cast<uint16_t>(command) & 0xff;

    _cs = 0;

    if (_spi->write(data, WORD_LENGTH, nullptr, 0) < 0) {
        _cs = 1;
        return -1;
    }

    _cs = 1;

    if (value) {
        _cs = 0;

        if (_spi->write(receiv, WORD_LENGTH, receiv, WORD_LENGTH) < 0) {
            _cs = 1;
            return -1;
        }

        _cs = 1;

        *value = ((uint16_t)receiv[0] << 8) | ((uint16_t)receiv[1] & 0x00FF);
    }

    return 0;
}

} // namespace sixtron
