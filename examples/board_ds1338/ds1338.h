/*
 ds1338.h

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

#ifndef DS1338_H
#define DS1338_H

/* DS1338 I2C Address (Alt. addr. pin grounded) */
#define DS1338			0xD0
#define DS1338_SECONDS		0x00
#define DS1338_MINUTES		0x01
#define DS1338_HOURS		0x02
#define DS1338_DAY		0x03
#define DS1338_DATE		0x04
#define DS1338_MONTH		0x05
#define DS1338_YEAR		0x06
#define DS1338_CONTROL		0x07

// Control register settings: 1Hz square wave out
#define DS1338_CONTROL_SETTING 0b10010000

// 4kHz
//#define DS1338_CONTROL_SETTING 0b10010001

// 8 kHz
//#define DS1338_CONTROL_SETTING 0b10010010

// 32kHz
//#define DS1338_CONTROL_SETTING 0b10010011

typedef struct ds1338_time_t
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t date;
	uint8_t month;
	uint8_t year;
} ds1338_time_t;

void
ds1338_init (void);
void
ds1338_write_register (const uint8_t address, const uint8_t value);
uint8_t
ds1338_return_register (const uint8_t reg);
void
ds1338_get_time (ds1338_time_t * const time);
void
ds1338_set_time (const ds1338_time_t * const time);

#endif //DS1338_H
