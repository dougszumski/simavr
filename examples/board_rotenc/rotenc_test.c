/*
	rotenc_test.c

	Copyright 2014 Doug Szumski <d.s.szumski@gmail.com>

	Based on i2ctest.c by:

	Copyright 2008-2011 Michel Pollet <buserror@gmail.com>

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
#include <libgen.h>
#include <pthread.h>
#include <curses.h>

#include "sim_avr.h"
#include "sim_gdb.h"
#include "sim_elf.h"
#include "rot_enc.h"
#include "avr_ioport.h"
#include "sim_vcd_file.h"

avr_t * avr = NULL;
rot_enc_t rot_enc;
avr_vcd_t vcd_file;

uint8_t do_button_press = 0;
volatile int state = cpu_Running;

static void *
avr_run_thread(void * ignore)
{
	state = cpu_Running;

	int b_press = do_button_press;

	while ((state != cpu_Done) && (state != cpu_Crashed)) {

		state = avr_run(avr);

		// TODO Check if we really need this
		if (do_button_press != b_press) {
					b_press = do_button_press;
					rot_enc_button_press(&rot_enc);
				}


	}

	endwin ();
	return NULL;
}

/*
 * Drives the 'rotary encoder' from the keyboard.
 */
void key_poll() {

	// Initialise curses.
	if (initscr () == NULL)
	{
		fprintf (stderr, "Error initialising lib curses\n");
	}
	noecho ();

	printw ("ROTARY ENCODER (press q to quit):\n\n"
	        "Press 'j' for CCW turn \n"
	        "Press 'l' for CW turn \n"
	        "Press 'k' for button click \n\n");

	for (;;) {

		if (state != cpu_Running) {
			endwin ();
			return;
		}

		switch (getch ()) {
			case 'j':
				// TODO: Thread safe??
				rot_enc_twist (&rot_enc, ROT_ENC_CCW_CLICK);
				break;
			case 'k':
				do_button_press++;
				break;
			case 'l':
				rot_enc_twist (&rot_enc, ROT_ENC_CW_CLICK);
				break;
			case 'q':
				endwin ();
				return;
			default:
				// Ignore
				break;
		}
	};
}

int main(int argc, char *argv[])
{
	elf_firmware_t f;
	const char * fname =  "atmega32_rotenc_test.axf";

	printf("Firmware pathname is %s\n", fname);
	elf_read_firmware(fname, &f);

	printf("firmware %s f=%d mmcu=%s\n", fname, (int)f.frequency, f.mmcu);

	avr = avr_make_mcu_by_name(f.mmcu);
	if (!avr) {
		fprintf(stderr, "%s: AVR '%s' not known\n", argv[0], f.mmcu);
		exit(1);
	}
	avr_init(avr);
	avr_load_firmware(avr, &f);

	/*
	avr->gdb_port = 1234;
	avr->state = cpu_Stopped;
	avr_gdb_init(avr);
	avr->state = cpu_Running; */

	// TODO: Use this
	/*typedef struct avr_iopin_t {
		uint16_t port : 8;			///< port e.g. 'B'
		uint16_t pin : 8;		///< pin number
	} avr_iopin_t;
	#define AVR_IOPIN(_port, _pin)	{ .port = _port, .pin = _pin } */

	rot_enc_init(avr, &rot_enc, "rotenc.1");

	// RE GPIO
	avr_connect_irq(rot_enc.irq + IRQ_ROT_ENC_OUT_A_PIN, avr_io_getirq(avr,
		        AVR_IOCTL_IOPORT_GETIRQ('A'), 0));

	// RE INT
	avr_connect_irq(rot_enc.irq + IRQ_ROT_ENC_OUT_B_PIN, avr_io_getirq(avr,
			        AVR_IOCTL_IOPORT_GETIRQ('D'), 2));

	// INT (button)
	// Pull up
	avr_raise_irq(rot_enc.irq + IRQ_ROT_ENC_OUT_BUTTON_PIN, 1);
	avr_connect_irq(rot_enc.irq + IRQ_ROT_ENC_OUT_BUTTON_PIN, avr_io_getirq(avr,
				        AVR_IOCTL_IOPORT_GETIRQ('B'), 2));

	// TODO add attach method i2c_eeprom_attach(avr, &ee, AVR_IOCTL_TWI_GETIRQ(0));
	// TODO add verbose ee.verbose = 1;

	avr_vcd_init(avr, "gtkwave_output.vcd", &vcd_file, 1000 /* usec */);
	//avr_vcd_add_signal(&vcd_file, avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), 2), 1 , "INT0");
	avr_vcd_add_signal(&vcd_file, rot_enc.irq + IRQ_ROT_ENC_OUT_A_PIN, 1, "A");
	avr_vcd_add_signal(&vcd_file, rot_enc.irq + IRQ_ROT_ENC_OUT_B_PIN, 1, "B");
	avr_vcd_start(&vcd_file);

	pthread_t run;
	pthread_create(&run, NULL, avr_run_thread, NULL);

	// Pull up (Sets up state to avoid pressing twice- is this correct?)
	avr_raise_irq(rot_enc.irq + IRQ_ROT_ENC_OUT_BUTTON_PIN, 1);

	// Start the encoder at phase 1
	avr_raise_irq(rot_enc.irq + IRQ_ROT_ENC_OUT_A_PIN, 0);
	avr_raise_irq(rot_enc.irq + IRQ_ROT_ENC_OUT_B_PIN, 0);

	key_poll();
}
