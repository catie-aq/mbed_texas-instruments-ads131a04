/*
 * Copyright (c) 2021, CATIE, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ads131a04/ads131a04.h"

namespace sixtron {

ADS131A04::ADS131A04()
{
}

ADS131A04::reset()
{
}

uint8_t ADS131A04::init();
{
    reset();

    uint8_t reg = 0;

    spi_read_command(RegisterAddress::ID_LSB, &reg);

    if (reg != CHIP_ID) {
        ThisThread::sleep_for(1s);
        i2c_read_register(RegisterAddress::ChipId, &reg);
        if (reg != CHIP_ID) {
            return -1;
        }
    }

    uint16_t status = 0;
    
    send_command(Command::NULL, &status);

    if(status == 0xff04)
    {
        status = read_command(Command::UNLOCK);

        if(status == 0x0655)
        {
            spi_write_register(RegisterAddress::CLK1, 0x02); // CLKIN/2

            spi_write_register(RegisterAddress::CLK2, 0x25); // FMOD/512 | ICLK/2

            // Set GAIN=2 for all ADCs
            spi_write_register(RegisterAddress::ADC1, 0x01); // TODO Move this on a function
            spi_write_register(RegisterAddress::ADC2, 0x01); // TODO Move this on a function
            spi_write_register(RegisterAddress::ADC3, 0x01); // TODO Move this on a function
            spi_write_register(RegisterAddress::ADC4, 0x01); // TODO Move this on a function

            // Enable All ADC channel
            spi_write_register(RegisterAddress::ADC_ENA, 0x0F);

            // Wake-up from standby mode
            send_command(Command::WAKEUP);

            send_command(Command::LOCK);
        }
    }

    return 0;
}

uint8_t ADS131A04::spi_read_register(RegisterAddress registerAddress, uint8_t *value)
{

}

uint8_t ADS131A04::spi_write_register(RegisterAddress registerAddress, uint8_t value)
{

}

uint8_t ADS131A04::send_command(Command command, uint16_t *value=nullptr)
{
    
}

} // namespace sixtron

