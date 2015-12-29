#ifndef _ISR_H_
#define _ISR_H_

#include <kernel/types.h>

/** Keyboard **/

#define KEYBOARD_DATA_PORT              0x60
#define KEYBOARD_Z_SCANCODE		0x2c
#define KEYBOARD_C_SCANCODE		0x2e
#define KEYBOARD_LEFT_SHIFT_SCANCODE    0x2a
#define KEYBOARD_RIGHT_SHIFT_SCANCODE   0x36
#define KEYBOARD_CTRL_SCANCODE          0x1d
#define KEYBOARD_ALT_SCANCODE           0x38
#define KEYBOARD_F1_SCANCODE		0x3b

/** Error code flags for page fault exception **/

#define EXC_PF_PRESENT  0x01
#define EXC_PF_WRITE    0x02
#define EXC_PF_USER     0x04

/** Macro to dump stack **/

#define dump_stack() { \
	ui32_t i; \
	ui32_t *ebp; \
	\
	asm volatile("mov %%ebp, %0" : "=m"(ebp)); \
	\
	printk("stack:\n"); \
	\
	for(i = 0; i < 20; i++) \
	{ \
		printk("%x\n", ebp[i]); \
	} \
}

/** Global variables. **/

#ifdef _ISR_C_
clock_t tics = 0;
#else
extern clock_t tics;
#endif

/** Exceptions **/

void _isr_default_exc();
/****************************************************************/
void _isr_pf_exc();

/** IRQs **/

void _isr_default_pic1_irq();
/****************************************************************/
void _isr_default_pic2_irq();
/****************************************************************/
void _isr_pit_irq();
/****************************************************************/
void _isr_kbd_irq();
/****************************************************************/
void _isr_0x09_irq();
/****************************************************************/
void _isr_0x0a_irq();
/****************************************************************/
void _isr_0x0b_irq();

/** Function pointers **/

#ifdef _ISR_C_
void (*_isr_0x09_irq_handler)() = 0;
void (*_isr_0x0a_irq_handler)() = 0;
void (*_isr_0x0b_irq_handler)() = 0;
#else
extern void (*_isr_0x09_irq_handler)();
extern void (*_isr_0x0a_irq_handler)();
extern void (*_isr_0x0b_irq_handler)();
#endif

#endif
