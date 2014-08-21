/*
	ds1338_virt.c

	Copyright 2014 Doug Szumski <d.s.szumski@gmail.com>

	Based on i2c_eeprom example by:

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

#include "avr_twi.h"
#include "ds1338_virt.h"
#include "sim_time.h"

// Square wave out prescaler modes; see p11 of DS1338 datasheet.
enum {
	DS1338_VIRT_PRESCALER_DIV_32768 = 0,
	DS1338_VIRT_PRESCALER_DIV_8,
	DS1338_VIRT_PRESCALER_DIV_4,
	DS1338_VIRT_PRESCALER_OFF,
};

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
 * Update the system behavior after a control register is written to
 */
static void
ds1338_virt_update (const ds1338_virt_t * const p)
{
	// The address of the register which was just updated
	switch (p->reg_addr)
	{
		case DS1338_VIRT_SECONDS:
			if (ds1338_get_flag(p->nvram[p->reg_addr], DS1338_VIRT_CH) == 0) {
				printf("DS1338 clock ticking\n");
			} else {
				printf("DS1338 clock stopped\n");
			}
			break;
		case DS1338_VIRT_CONTROL:
			printf("DS1338 control register updated\n");
			// TODO: Check if changing the prescaler resets the clock counter
			// and if so do it here?
			break;
		default:
			// No control register updated
			return;
	}
}

static void
ds1338_virt_tick_time(ds1338_virt_t *p) {



}

static void
ds1338_virt_square_wave_output_invert (ds1338_virt_t *p)
{
	if(!ds1338_get_flag(p->nvram[DS1338_VIRT_CONTROL], DS1338_VIRT_SQWE)) {
		// Square wave output disabled
		return;
	}

	p->square_wave = !p->square_wave;
	// TODO: Fire event
	if (p->square_wave) {
		printf ("Tick\n");
	} else {
		printf ("Tock\n");
	}
}

static avr_cycle_count_t
ds1338_virt_clock_tick (struct avr_t * avr,
                   avr_cycle_count_t when,
                   ds1338_virt_t *p)
{
	avr_cycle_count_t next_tick = when + avr_usec_to_cycles (avr, DS1338_CLK_PERIOD_US / 2);

	if (!ds1338_get_flag (p->nvram[DS1338_VIRT_SECONDS], DS1338_VIRT_CH)) {
		// Oscillator is enabled. Note that this counter is allowed to wrap.
		p->rtc++;
	} else {
		// Avoid a condition match below with the clock switched off
		return next_tick;
	}

	/*
	 * Update the time
	 */
	if (p->rtc == 0) {
		// 1 second has passed
		ds1338_virt_tick_time(p);
	}

	/*
	 * Deal with the square wave output
	 */
	uint8_t prescaler_mode = ds1338_get_flag (p->nvram[DS1338_VIRT_CONTROL],
	                                          DS1338_VIRT_RS0)
	                      + (ds1338_get_flag (p->nvram[DS1338_VIRT_CONTROL],
	                                          DS1338_VIRT_RS1) << 1);

	switch (prescaler_mode)
	{
		case DS1338_VIRT_PRESCALER_DIV_32768:
			if ((p->rtc + 1) % DS1338_CLK_FREQ == 0) {
				//printf("DS1338 COUNTER: %d\n", p->rtc + 1);
				ds1338_virt_square_wave_output_invert(p);
			}
			break;
		case DS1338_VIRT_PRESCALER_DIV_8:
			if ((p->rtc + 1) % (DS1338_CLK_FREQ / 8) == 0)
				ds1338_virt_square_wave_output_invert(p);
			break;
		case DS1338_VIRT_PRESCALER_DIV_4:
			if ((p->rtc + 1) % (DS1338_CLK_FREQ / 4) == 0)
				ds1338_virt_square_wave_output_invert(p);
			break;
		case DS1338_VIRT_PRESCALER_OFF:
			ds1338_virt_square_wave_output_invert(p);
			break;
		default:
			// Invalid mode
			break;
	}

	return next_tick;
}

static void
ds1338_virt_clock_xtal_init(
		struct avr_t * avr,
		ds1338_virt_t *p)
{
	p->rtc = 0;

	/*
	 * Set a timer for half the clock period to allow reconstruction
	 * of the square wave output at the maximum possible frequency.
	 */
	avr_cycle_timer_register_usec(avr,
	                              DS1338_CLK_PERIOD_US / 2,
	                              (void *) ds1338_virt_clock_tick,
	                              p);

	printf("DS1338 clock crystal period %duS or %d cycles\n",
			DS1338_CLK_PERIOD_US,
			(int)avr_usec_to_cycles(avr, DS1338_CLK_PERIOD_US));
}

/*
 * Called when a RESET signal is sent
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

	//print("DS1338 addr: 0x%02x, mask: 0x%02x, twi: 0x%02x\n", p->addr_base, p->addr_mask, v.u.twi.addr);

	/*
	 * If we receive a STOP, check it was meant to us, and reset the transaction
	 */
	if (v.u.twi.msg & TWI_COND_STOP) {
		if (p->selected) {
			// it was us !
			if (p->verbose)
				printf("DS1338 stop\n\n");
		}
		/* We should not zero the register address here because read mode uses the last
		 * register address stored and write mode always overwrites it.
		 */
		p->selected = 0;
		p->reg_selected = 0;

	}

	/*
	 * If we receive a start, reset status, check if the slave address is
	 * meant to be us, and if so reply with an ACK bit
	 */
	if (v.u.twi.msg & TWI_COND_START) {
		//printf("DS1338 start attempt: 0x%02x, mask: 0x%02x, twi: 0x%02x\n", p->addr_base, p->addr_mask, v.u.twi.addr);
		p->selected = 0;
		// Ignore the read write bit
		if ((v.u.twi.addr >> 1) ==  (DS1338_VIRT_TWI_ADDR >> 1)) {
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
		// Write transaction
		if (v.u.twi.msg & TWI_COND_WRITE) {
			// ACK the byte
			avr_raise_irq(p->irq + TWI_IRQ_INPUT,
					avr_twi_irq_msg(TWI_COND_ACK, p->selected, 1));
			// Write to the selected register (see p13. DS1388 datasheet for details)
			if (p->reg_selected) {
				printf("DS1338 set register 0x%02x to 0x%02x\n", p->reg_addr, v.u.twi.data);
				p->nvram[p->reg_addr] = v.u.twi.data;
				ds1338_virt_update(p);
				ds1338_virt_incr_addr(p);
			// No register selected so select one
			} else {
				printf("DS1338 select register 0x%02x\n",  v.u.twi.data);
				p->reg_selected = 1;
				p->reg_addr = v.u.twi.data;
			}
		}
		// Read transaction
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
		ds1338_virt_t * p )
{
	memset(p, 0, sizeof(*p));
	memset(p->nvram, 0x00, sizeof(p->nvram));

	p->irq = avr_alloc_irq(&avr->irq_pool, 0, 2, _ds1338_irq_names);
	avr_irq_register_notify(p->irq + TWI_IRQ_OUTPUT, ds1338_virt_in_hook, p);

	// Start with the oscillator disabled, at least until there is some "battery backup"
	p->nvram[DS1338_VIRT_SECONDS] |=  (1 << DS1338_VIRT_CH);

	ds1338_virt_clock_xtal_init(avr, p);
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
