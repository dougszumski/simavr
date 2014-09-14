/*
	rot_enc.h

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

/*
 * This parts models a rotary encoder with a push button.
 */

#ifndef __ROT_ENC_H__
#define __ROT_ENC_H__

#include "rot_enc.h"

#define ROTENC_CLICK_DURATION_US 100000
#define ROTENC_BUTTON_DURATION_US 1000000
#define ROTENC_STATE_COUNT 4

typedef enum {
	ROT_ENC_CW_CLICK = 0,
	ROT_ENC_CCW_CLICK
} rotenc_twist_t;

typedef struct rotenc_pin_state_t {
	uint8_t a_pin;
	uint8_t b_pin;
} rotenc_pins_t;

enum {
	IRQ_ROT_ENC_OUT_A_PIN = 0,
	IRQ_ROT_ENC_OUT_B_PIN,
	IRQ_ROT_ENC_OUT_BUTTON_PIN,
	IRQ_ROT_ENC_COUNT
};

typedef struct rot_enc_t {
	avr_irq_t * irq;	// output irq
	struct avr_t * avr;
	rotenc_twist_t direction;
	uint8_t phase;		// current index in phase table
} rot_enc_t;

void
rot_enc_init (struct avr_t * avr,
              rot_enc_t * b,
              const char * name);

void
rot_enc_twist (rot_enc_t * rot_enc,
               rotenc_twist_t direction);

void
rot_enc_button_press(rot_enc_t * rot_enc);

#endif /* __ROT_ENC_H__*/
