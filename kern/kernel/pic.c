/****************************************************************
 * pic.c                                                        *
 *                                                              *
 *    PIC initialization.                                       *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <kernel/io.h>
#include <kernel/types.h>

#include "pic.h"

void pic_init()
{
	ui16_t div = 1193182 / CLK_FREQ;

	/** Using ICW4 **/

	outbt(PIC1_BASE, PIC_ICW1_DEFAULT | PIC_ICW1_WITH_ICW4);
	outbt(PIC2_BASE, PIC_ICW1_DEFAULT | PIC_ICW1_WITH_ICW4);

	/** Origin interrupt vector is 0x20 for master, 0x70 for slave. **/

	outbt(PIC1_BASE + 1, 0x20);
	outbt(PIC2_BASE + 1, 0x70);

	/** Slave is connected to pin 2 of the master. **/

	outbt(PIC1_BASE + 1, 1 << 2);
	outbt(PIC2_BASE + 1, 0x02);

	/** Use default configuration for ICW4. **/

	outbt(PIC1_BASE + 1, PIC_ICW4_DEFAULT);
	outbt(PIC2_BASE + 1, PIC_ICW4_DEFAULT);

	/** Unmasking all interrupts. **/

	outbt(PIC1_BASE + 1, 0);
	outbt(PIC2_BASE + 1, 0);

	/** Set the clock frequency. **/

	outb(0x43, 0x36);
	outb(0x40, (ui8_t)div);
	outb(0x40, (ui8_t)(div >> 8));
}
