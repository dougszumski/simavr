/*
	atmega32_rotenc_test.c

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
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>

#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "atmega32");

volatile uint8_t go = 1;

/* Rotary encoder spin uses INTO on PD2. Pull up on PCB*/
void rotary_spin_int_init(void)
{
	cli();
	// Set INT0 and GPIO pin as inputs
	DDRD &= ~(1 << PD2) & ~(1 << PA0);
	// Any logical change on INT0 generates int. p.67
	MCUCR |= (1 << ISC00);
	MCUCR &= ~(1 << ISC01);
	// Enable interrupt pin PD2
	GICR |= (1 << INT0);
	sei();
}

/* Rotary encoder click uses INT2 on PB2. Pullup is on PCB.*/
void rotary_click_int_init(void)
{
	// Avoid triggering interrupts when setting ISC2 p67.
	cli();
	DDRB &= ~(1 << PB2);
	// Falling edge trigger (pin is pulled up) p67.
	MCUCSR &= ~(1 << ISC2);
	//MCUCSR |= (1 << ISC2);
	// Enable interrupt pin PB2
	GICR |= (1 << INT2);
	sei();
}

int main()
{
	rotary_spin_int_init();
	rotary_click_int_init();

	while(go) {

	}

	cli();
	sleep_mode();
}

/* Interrupt on twist. Device connected to D2 (INT0), and A0 (GPIO).
 *
 * The datasheet has a diagram of the phase vs physical click.
 */
ISR(INT0_vect)
{
	static uint8_t prev_state;

	// Current state of encoder, format: 0b00000B0A
	uint8_t curr_state = (PIND & (1 << PD2)) | (PINA & (1 << PA0));

	// XOR current state of D2 with previous state of A0
	uint8_t clockwise_twist = ((curr_state & (1 << PD2)) >> 2) ^ (prev_state & (1 << PA0));

	if (clockwise_twist) {
		go++;
	} else {
		go--;
	}

	prev_state = curr_state;

}

// Interrupt on button press (connected to pin B2)
ISR(INT2_vect)
{
	go = 0;
}

