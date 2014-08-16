/*
	ds1338_virt.c

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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sim_avr.h"
#include "avr_twi.h"
#include "ds1338_virt.h"

/*
 * Increment the ds1338 register address.
 */
static void
ds1338_virt_incr_addr (ds1338_virt_t * const p)
{
	if (p->reg_addr < sizeof(p->nvram)) {
		p->reg_addr++;
	} else {
		// TODO Check if this wraps, or if it just stops incrementing
		p->reg_addr = 0;
	}
}


/*
 * called when a RESET signal is sent
 */
static void
ds1338_virt_in_hook(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
	ds1338_virt_t * p = (ds1338_virt_t*)param;
	avr_twi_msg_irq_t v;
	v.u.v = value;

	//	print("DS1338 addr: 0x%02x, mask: 0x%02x, twi: 0x%02x\n", p->addr_base, p->addr_mask, v.u.twi.addr);

	/*
	 * If we receive a STOP, check it was meant to us, and reset the transaction
	 */
	if (v.u.twi.msg & TWI_COND_STOP) {
		if (p->selected) {
			// it was us !
			if (p->verbose)
				printf("DS1338 stop\n\n");
		}
		p->selected = 0;
		p->reg_selected = 0;

		// We should not zero the register address here because read mode uses the last
		// register address stored and write mode always overwrites it.
	}
	/*
	 * if we receive a start, reset status, check if the slave address is
	 * meant to be us, and if so reply with an ACK bit
	 */
	if (v.u.twi.msg & TWI_COND_START) {

		p->selected = 0;
		if ((p->addr_base & ~p->addr_mask) == (v.u.twi.addr & ~p->addr_mask)) {
			// it's us !
			if (p->verbose)
				printf("DS1338 start\n");
			p->selected = v.u.twi.addr;
			avr_raise_irq(p->irq + TWI_IRQ_INPUT,
					avr_twi_irq_msg(TWI_COND_ACK, p->selected, 1));
		}
	}
	/*
	 * If it's a data transaction, first check it is meant to be us (we
	 * received the correct address and are selected)
	 */
	if (p->selected) {
		/*
		 * This is a write transaction, first receive as many address bytes
		 * as we need, then set the address register, then start
		 * writing data,
		 */
		if (v.u.twi.msg & TWI_COND_WRITE) {

			//printf("DS1338 WRITE data: 0x%02x\n", v.u.twi.data);

			// Every byte written must be ACKed
			avr_raise_irq(p->irq + TWI_IRQ_INPUT,
					avr_twi_irq_msg(TWI_COND_ACK, p->selected, 1));

			// See p13 of DS for how this works
			if (p->reg_selected) {
				printf("DS1338 set register 0x%02x to 0x%02x\n", p->reg_addr, v.u.twi.data);
				p->nvram[p->reg_addr] = v.u.twi.data;
				ds1338_virt_incr_addr(p);

			} else {
				printf("DS1338 select register 0x%02x\n",  v.u.twi.data);
				p->reg_selected = 1;
				p->reg_addr = v.u.twi.data;
			}
		}
		/*
		 * It's a read transaction, just send the next byte back to the master
		 */
		if (v.u.twi.msg & TWI_COND_READ) {
			printf("DS1338 READ data at 0x%02x: 0x%02x\n", p->reg_addr, p->nvram[p->reg_addr]);
			uint8_t data = p->nvram[p->reg_addr];
			ds1338_virt_incr_addr(p);
			avr_raise_irq(p->irq + TWI_IRQ_INPUT,
					avr_twi_irq_msg(TWI_COND_READ, p->selected, data));
		}
	}
}

static const char * _ds1338_irq_names[2] = {
		[TWI_IRQ_INPUT] = "8>ds1338.out",
		[TWI_IRQ_OUTPUT] = "32<ds1338.in",
};

void
ds1338_virt_init(
		struct avr_t * avr,
		ds1338_virt_t * p,
		uint8_t addr,
		uint8_t mask)
{
	memset(p, 0, sizeof(*p));
	memset(p->nvram, 0x00, sizeof(p->nvram));

	p->addr_base = addr;
	p->addr_mask = mask;

	p->irq = avr_alloc_irq(&avr->irq_pool, 0, 2, _ds1338_irq_names);
	avr_irq_register_notify(p->irq + TWI_IRQ_OUTPUT, ds1338_virt_in_hook, p);
}

void
ds1338_virt_attach(
		struct avr_t * avr,
		ds1338_virt_t * p,
		uint32_t i2c_irq_base )
{
	// "connect" the IRQs of the DS1338 to the TWI/i2c master of the AVR
	avr_connect_irq(
		p->irq + TWI_IRQ_INPUT,
		avr_io_getirq(avr, i2c_irq_base, TWI_IRQ_INPUT));
	avr_connect_irq(
		avr_io_getirq(avr, i2c_irq_base, TWI_IRQ_OUTPUT),
		p->irq + TWI_IRQ_OUTPUT );
}
