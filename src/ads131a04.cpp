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

    if (status == STATUS_INIT) {
        return 0;
    }

    return -1;
}

int8_t ADS131A04::start()
{
    uint16_t status = 0;

    send_command(Command::unlock, &status);

    if (status == STATUS_OK) {
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

    if (status == STATUS_OK) {
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

    if (status == STATUS_OK) {
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

int8_t ADS131A04::set_frequency(uint8_t clkin_div, uint8_t ficlk_div, uint8_t fmod_div)
{
    if ((clkin_div == 0) | (clkin_div > CLKIN_DIV_14))
        return -1;
    if ((ficlk_div == 0) | (clkin_div > FICLK_DIV_14))
        return -1;
    if (fmod_div > OSR_32)
        return -1;

    uint16_t status = 0;
    uint8_t clk1_reg = 0x00 + (clkin_div << 1); // see datasheet 9.6.1.12
    uint8_t clk2_reg = 0x00 + ((ficlk_div << 5) + fmod_div); // see datasheet 9.6.1.1
    printf("clk1_reg= 0x%02X, clk2_reg= 0x%02X\n", clk1_reg, clk2_reg);

    send_command(Command::unlock, &status);
    if (status == STATUS_OK) {
        spi_write_register(RegisterAddress::clk1, clk1_reg);
        spi_write_register(RegisterAddress::clk2, clk2_reg);
        send_command(Command::lock);
        return 0;
    } else {
        send_command(Command::lock);
        return -1;
    }
}

int8_t ADS131A04::set_frequency(Frequency freq)
{
    switch (freq) {
#if MBED_CONF_ADS131A04_QUARTZ_FREQUENCY == 16384
            // Ref CLK =        16.384MHz
        case Frequency::_2500Hz: // 2560Hz // Noise on other channels + offset on gain
            return set_frequency(CLKIN_DIV_4, FICLK_DIV_2, OSR_800);
        case Frequency::_2000Hz: // 2000Hz
            return set_frequency(CLKIN_DIV_8, FICLK_DIV_8, OSR_128);
        case Frequency::_1000Hz: // 1000Hz
            return set_frequency(CLKIN_DIV_8, FICLK_DIV_8, OSR_256);
        case Frequency::_500Hz: // 500Hz
            return set_frequency(CLKIN_DIV_8, FICLK_DIV_8, OSR_512);
        case Frequency::_250Hz: // 250Hz
            return set_frequency(CLKIN_DIV_8, FICLK_DIV_2, OSR_4096);
#elif MBED_CONF_ADS131A04_QUARTZ_FREQUENCY == 20500 // 20500
            // Ref CLK =        20.500MHz
        case Frequency::_2500Hz: // 2502.4Hz // Best results, High OSR
            return set_frequency(CLKIN_DIV_2, FICLK_DIV_2, OSR_2048);
        case Frequency::_2000Hz: // 2002Hz
            return set_frequency(CLKIN_DIV_8, FICLK_DIV_10, OSR_128);
        case Frequency::_1000Hz: // 1001Hz
            return set_frequency(CLKIN_DIV_8, FICLK_DIV_10, OSR_256);
        case Frequency::_500Hz: // 500.5Hz
            return set_frequency(CLKIN_DIV_8, FICLK_DIV_10, OSR_512);
        case Frequency::_250Hz: // 250.2Hz
            return set_frequency(CLKIN_DIV_8, FICLK_DIV_10, OSR_1024);
#else
#error "quartz frequency not supported."
#endif
        default:
            return -1;
    }
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
