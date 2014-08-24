/*
 atmega32_ds1338.c

 Copyright 2014 Doug Szumski <d.s.szumski@gmail.com>

 This file is part of simavr.

 simavr is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 simavr is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#undef F_CPU
#define F_CPU 7380000

#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "atmega1280");

#include "ds1338.h"
#include "i2cmaster.h"

int
main ()
{
	i2c_init ();
	ds1338_init();

	ds1338_time_t time = {
			.date = 28,
			.day = 2,
			.hours = 3,
			.minutes = 4,
			.month = 2,
			.seconds = 6,
			.year = 14,
	};

	ds1338_set_time(&time);

	//ds1338_get_time(&time);

	//cli();
	//sleep_mode();



	/*
	 * Demonstrate the virtual part functionality.
	 */
	for (;;)
	{
	}

}
