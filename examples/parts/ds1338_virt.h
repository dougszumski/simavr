/*
	ds1338_virt.h

	Copyright 2014 Doug Szumski <d.s.szumski@gmail.com>
	Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

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

#ifndef DS1338_VIRT_H_
#define DS1338_VIRT_H_

#include "sim_irq.h"

// TWI address is fixed
#define DS1338_VIRT_TWI_ADDR		0xD0

/*
 * Registers. Time is in BCD. See p10 of the DS1388 datasheet.
 */
#define DS1338_VIRT_SECONDS		0x00
#define DS1338_VIRT_MINUTES		0x01
#define DS1338_VIRT_HOURS		0x02
#define DS1338_VIRT_DAY			0x03
#define DS1338_VIRT_DATE		0x04
#define DS1338_VIRT_MONTH		0x05
#define DS1338_VIRT_YEAR		0x06
#define DS1338_VIRT_CONTROL		0x07

/*
 * Seconds register flag - oscillator is enabled when
 * this is set to zero. Undefined on startup.
 */
#define DS1338_VIRT_CH			7

/*
 * Control register flags. See p11 of the DS1388 datasheet.
 *
 *  +-----+-----+-----+------------+------+
 *  | OUT | RS1 | RS0 | SQW_OUTPUT | SQWE |
 *  +-----+-----+-----+------------+------+
 *  | X   | 0   | 0   | 1Hz        |    1 |
 *  | X   | 0   | 1   | 4.096kHz   |    1 |
 *  | X   | 1   | 0   | 8.192kHz   |    1 |
 *  | X   | 1   | 1   | 32.768kHz  |    1 |
 *  | 0   | X   | X   | 0          |    0 |
 *  | 1   | X   | X   | 1          |    0 |
 *  +-----+-----+-----+------------+------+
 *
 *  OSF : Oscillator stop flag. Set to 1 when oscillator
 *        is interrupted.
 *
 *  SQWE : Square wave out, set to 1 to enable.
 */
#define DS1338_VIRT_RS0			0
#define DS1338_VIRT_RS1			1
#define DS1338_VIRT_SQWE		4
#define DS1338_VIRT_OSF			5
#define DS1338_VIRT_OUT			7

/*
 * DS1338 I2C clock
 *
 * TODO: Confirm DS1307 is pin compatible and any other similar ones.
 */
typedef struct ds1338_virt_t {
	avr_irq_t *	irq;		// irq list
	int verbose;

	uint8_t selected;		// selected address
	uint8_t reg_selected;		// register selected
	uint8_t reg_addr;		// register pointer
	uint8_t nvram[64];		// battery backed up NVRAM
	//uint8_t ctrl_reg;		// clock control register
} ds1338_virt_t;

void
ds1338_virt_init(
		struct avr_t * avr,
		ds1338_virt_t * p );

/*
 * Attach the ds1307 to the AVR's TWI master code,
 * pass AVR_IOCTL_TWI_GETIRQ(0) for example as i2c_irq_base
 */
void
ds1338_virt_attach(
		struct avr_t * avr,
		ds1338_virt_t * p,
		uint32_t i2c_irq_base );

/*static inline int
ds1338_set_flag (ds1338_virt_t *b, uint8_t bit, int val)
{
	int old = b->ctrl_reg & (1 << bit);
	b->ctrl_reg = (b->ctrl_reg & ~(1 << bit)) | (val ? (1 << bit) : 0);
	return old != 0;
} */

static inline int
ds1338_get_flag (uint8_t reg, uint8_t bit)
{
	return (reg & (1 << bit)) != 0;
}

#endif /* DS1338_VIRT_H_ */
