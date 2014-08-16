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

/* DS1338_VIRT I2C Address (Alt. addr. pin grounded) */
#define DS1338_VIRT_ADDR		0xD0

#define DS1338_VIRT_SECONDS		0x00
#define DS1338_VIRT_MINUTES		0x01
#define DS1338_VIRT_HOURS		0x02
#define DS1338_VIRT_DAY			0x03
#define DS1338_VIRT_DATE		0x04
#define DS1338_VIRT_MONTH		0x05
#define DS1338_VIRT_YEAR		0x06
#define DS1338_VIRT_CONTROL		0x07


#include "sim_irq.h"

/*
 * This is a generic i2c eeprom; it can be up to 4096 bytes, and can work
 * in two modes :
 * 1) ONE slave address, and either one or two bytes sent on i2c to specify
 *    the byte to read/write.
 *    So a transaction looks like:
 *    <i2c address> [<byte offset MSB>] <byte offset LSB> [<data>]
 * 2) Multiple slave address to specify the high byte offset value, and one
 *    byte offset sent
 *    So a transaction looks like:
 *    <i2c address; x low bits used as byte offset> <byte offset LSB> [<data>]
 *
 * these two modes seem to cover many eeproms
 */
typedef struct ds1338_virt_t {
	avr_irq_t *	irq;		// irq list
	uint8_t addr_base;
	uint8_t addr_mask;
	int verbose;

	uint8_t selected;		// selected address
	uint8_t reg_selected;		// register selected
	uint8_t reg_addr;		// read/write address register
	uint8_t nvram[64];		// battery backed up NVRAM
} ds1338_virt_t;

/*
 * Initializes an eeprom.
 *
 * The address is the TWI/i2c address base, for example 0xa0 -- the 7 MSB are
 * relevant, the bit zero is always meant as the "read write" bit.
 * The "mask" parameter specifies which bits should be matched as a slave;
 * if you want to have a peripheral that handle read and write, use '1'; if you
 * want to also match several addresses on the bus, specify these bits on the
 * mask too.
 * Example:
 * Address 0xa1 mask 0x00 will match address 0xa0 in READ only
 * Address 0xa0 mask 0x01 will match address 0xa0 in read AND write mode
 * Address 0xa0 mask 0x03 will match 0xa0 0xa2 in read and write mode
 *
 * The "data" is optional, data is initialized as 0xff like a normal eeprom.
 */
void
ds1338_virt_init(
		struct avr_t * avr,
		ds1338_virt_t * p,
		uint8_t addr,
		uint8_t mask);

/*
 * Attach the eeprom to the AVR's TWI master code,
 * pass AVR_IOCTL_TWI_GETIRQ(0) for example as i2c_irq_base
 */
void
ds1338_virt_attach(
		struct avr_t * avr,
		ds1338_virt_t * p,
		uint32_t i2c_irq_base );

#endif /* DS1338_VIRT_H_ */
