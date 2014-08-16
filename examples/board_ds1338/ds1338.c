/*
 ds1338.c

 DS1338 I2C clock module driver

 Copyright 2014 Doug Szumski <d.s.szumski@gmail.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <stdint.h>
#include <util/delay.h>

#include "ds1338.h"
#include "i2cmaster.h"

static void decompose_time(uint8_t number, uint8_t *tens, uint8_t* units);

void ds1338_write_register(const uint8_t address, const uint8_t value)
{
	i2c_start(DS1338 + I2C_WRITE);
	i2c_start(DS1338 + I2C_WRITE);
	i2c_write(address);
	i2c_write(value);
	i2c_stop();
}

uint8_t ds1338_return_register(const uint8_t reg)
{
	uint8_t data;

	i2c_start(DS1338 + I2C_WRITE);
	i2c_write(reg);
	i2c_rep_start(DS1338 + I2C_READ);
	data = i2c_readNak();
	i2c_stop();

	return data;
}

void ds1338_init(void)
{
	uint8_t data;
	ds1338_write_register(DS1338_CONTROL, DS1338_CONTROL_SETTING);
	//Set the CS bit to zero to enable clock
	data = ds1338_return_register(DS1338_SECONDS);
	data &= 0b01111111;
	ds1338_write_register(DS1338_SECONDS, data);
	//TODO if data is crazy initialise with zeros to stop weird settings in menu

}


void ds1338_get_time(ds1338_time_t * const time)
{
	uint8_t data;
	//Update the time all at once
	//TODO update only on rollover proximity?
	//If not read the whole lot at once instead of in parts

	data = ds1338_return_register(DS1338_SECONDS);
	time->seconds = 10 * ((data & 0b01110000) >> 4) + (data & 0x0F);

	data = ds1338_return_register(DS1338_MINUTES);
	time->minutes = 10 * ((data & 0b01110000) >> 4) + (data & 0x0F);

	data = ds1338_return_register(DS1338_HOURS);
	// TODO
	/*
	twelve_hour_mode = (data & 0b01000000) >> 6;
	if (twelve_hour_mode)
	{
		time->hours = 10 * ((data & 0b00010000) >> 4) + (data & 0x0F);
		pm = (data & 0b00100000) >> 5;
	}
	else
	{
		time->hours = 10 * ((data & 0b00110000) >> 4) + (data & 0x0F);
	} */

	data = ds1338_return_register(DS1338_DAY);
	time->day = (data & 0b00000111) - 1; //subtract 1 so 0-6

	data = ds1338_return_register(DS1338_DATE);
	time->date = 10 * ((data & 0b00110000) >> 4) + (data & 0x0F);

	data = ds1338_return_register(DS1338_MONTH);
	time->month = 10 * ((data & 0b00010000) >> 4) + (data & 0x0F);

	data = ds1338_return_register(DS1338_YEAR);
	time->year = 10 * ((data & 0xF0) >> 4) + (data & 0x0F);
}

/* TODO: Cleanup this mess -> set everything all at once with a buffer? */
void ds1338_set_time(const ds1338_time_t * const time)
{
	uint8_t tens, units, byte;

	//This should always write zero to the CS bit to enable the clock
	decompose_time(time->seconds, &tens, &units);
	ds1338_write_register(DS1338_SECONDS, ((tens << 4) | units));

	decompose_time(time->minutes, &tens, &units);
	ds1338_write_register(DS1338_MINUTES, ((tens << 4) | units));

	decompose_time(time->hours, &tens, &units);
	byte = (tens << 4) | units;

	// TODO
	/*
	if (twelve_hour_mode)
	{
		//12 hour mode
		//FIXME Bug here:
		//PM is set when bit 5 is high, 12 hour mode when bit 6 is high
		byte |= ((pm) << 5) | ((twelve_hour_mode) << 6);
	} */
	ds1338_write_register(DS1338_HOURS, byte);

	ds1338_write_register(DS1338_DAY, time->day + 1);

	decompose_time(time->date, &tens, &units);
	ds1338_write_register(DS1338_DATE, ((tens << 4) | units));

	decompose_time(time->month, &tens, &units);
	ds1338_write_register(DS1338_MONTH, ((tens << 4) | units));

	decompose_time(time->year, &tens, &units);
	ds1338_write_register(DS1338_YEAR, ((tens << 4) | units));
}

void decompose_time(uint8_t number, uint8_t *tens, uint8_t* units)
{
	*tens = 0;
	*units = 0;
	while (number >= 10)
	{
		(*tens)++;
		number -= 10;
	}
	while (number >= 1)
	{
		(*units)++;
		number -= 1;
	}
}
