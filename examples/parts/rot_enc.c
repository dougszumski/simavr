/*
	rot_enc.c

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sim_avr.h"
#include "rot_enc.h"

const rotenc_pins_t state_table[ROTENC_STATE_COUNT] = {
	{ 0, 0 },
	{ 1, 0 },
	{ 1, 1 },
	{ 0, 1 }
};

static avr_cycle_count_t
rot_enc_state_change(
		avr_t * avr,
		avr_cycle_count_t when,
		void * param)
{
	rot_enc_t * rot_enc = (rot_enc_t *)param;

	switch(rot_enc->direction) {
		case ROT_ENC_CW_CLICK:
			// Advance phase forwards
			if (rot_enc->phase < ROTENC_STATE_COUNT) {
				rot_enc->phase++;
			} else {
				rot_enc->phase = 0;
			}
			printf("ROT_ENC: CW click\n\r");
			break;
		case ROT_ENC_CCW_CLICK:
			// Advance phase backwards
			if (rot_enc->phase > 0) {
				rot_enc->phase--;
			} else {
				rot_enc->phase = ROTENC_STATE_COUNT;
			}
			printf("ROT_ENC: CCW click\n\r");
			break;
		default:
			// Invalid direction
			break;
	}

	avr_raise_irq(rot_enc->irq + IRQ_ROT_ENC_OUT_A_PIN, state_table[rot_enc->phase].a_pin);
	avr_raise_irq(rot_enc->irq + IRQ_ROT_ENC_OUT_B_PIN, state_table[rot_enc->phase].b_pin);

	return 0;
}

static avr_cycle_count_t
rot_enc_button_auto_release(
		avr_t * avr,
		avr_cycle_count_t when,
		void * param)
{
	rot_enc_t * rot_enc = (rot_enc_t *)param;
	avr_raise_irq(rot_enc->irq + IRQ_ROT_ENC_OUT_BUTTON_PIN, 1);
	printf("ROT_ENC: Button release\n\r");
	return 0;
}

void
rot_enc_button_press(rot_enc_t * rot_enc)
{
	avr_cycle_timer_cancel(rot_enc->avr, rot_enc_button_auto_release, rot_enc);
	// Press down
	printf("ROT_ENC: Button press\n\r");
	avr_raise_irq(rot_enc->irq + IRQ_ROT_ENC_OUT_BUTTON_PIN, 0);
	// Pull up later
	avr_cycle_timer_register_usec(rot_enc->avr, ROTENC_BUTTON_DURATION_US, rot_enc_button_auto_release, rot_enc);
}

void
rot_enc_twist(rot_enc_t * rot_enc, rotenc_twist_t direction)
{
	rot_enc->direction = direction;

	// TODO: Make sure all of these are cancelled.
	avr_cycle_timer_cancel(rot_enc->avr, rot_enc_state_change, rot_enc);

	// Half way to detent
	avr_cycle_timer_register_usec(rot_enc->avr, ROTENC_CLICK_DURATION_US/2, rot_enc_state_change, rot_enc);

	// Detent point
	//avr_cycle_timer_register_usec(rot_enc->avr, ROTENC_CLICK_DURATION_US, rot_enc_state_change, rot_enc);
}

void
rot_enc_init(
		avr_t *avr,
		rot_enc_t * rot_enc,
		const char * name)
{
	//memset(rot_enc, 0, sizeof(*rot_enc));

	rot_enc->irq = avr_alloc_irq(&avr->irq_pool, 0, IRQ_ROT_ENC_COUNT, &name);
	rot_enc->avr = avr;
}

