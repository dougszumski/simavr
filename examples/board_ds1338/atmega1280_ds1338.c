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

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdio.h>

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
			.date = 31,
			.day = 6,
			.hours = 23,
			.minutes = 59,
			.month = 12,
			.seconds = 50,
			.year = 14,
	};

	ds1338_set_time(&time);

	/*
	 * Demonstrate the virtual part functionality.
	 */
	for(;;) {
		_delay_ms(100);
		ds1338_get_time(&time);
		if (time.seconds == 10)
			break;
	}

	cli();
	sleep_mode();

}
