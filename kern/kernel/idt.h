#ifndef _IDT_H_
#define _IDT_H_

#include <kernel/types.h>

/** IDT descriptor **/

struct idt_descriptor
{
	ui16_t offset_0_15;
	ui16_t selector;
	ui8_t zero;
	ui8_t type:4;
	ui8_t attr:4;
	ui16_t offset_16_31;
} __attribute__ ((packed));

/** IDT description **/

struct idt_description
{
	ui16_t limit;
	ui32_t base;
} __attribute__ ((packed));

/** Type **/

#define IDT_TYPE_32_TASK_GATE	0x5
#define IDT_TYPE_16_INT_GATE	0x6
#define IDT_TYPE_16_TRAP_GATE	0x7
#define IDT_TYPE_32_INT_GATE	0xe
#define IDT_TYPE_32_TRAP_GATE	0xf

/** Attribute flags **/

#define IDT_ATTRIBUTE_P		0x8
#define IDT_ATTRIBUTE_RING0	0x0
#define IDT_ATTRIBUTE_RING1	0x2
#define IDT_ATTRIBUTE_RING2	0x4
#define IDT_ATTRIBUTE_RING3	0x6
#define IDT_ATTRIBUTE_S		0x1

/** Functions **/

void idt_init();
/****************************************************************/
void idt_init_descriptor(struct idt_descriptor *desc,
                         ui32_t offset,
                         ui8_t type,
                         ui8_t attr,
                         ui16_t selector);

/** Interrupt wrappers entry (implemented in kernel/isr_wrap.asm) **/

extern void isr_default_exc();
/****************************************************************/
extern void isr_pf_exc();
/****************************************************************/
extern void isr_default_pic1_irq();
/****************************************************************/
extern void isr_default_pic2_irq();
/****************************************************************/
extern void isr_pit_irq();
/****************************************************************/
extern void isr_kbd_irq();
/****************************************************************/
extern void isr_0x09_irq();
/****************************************************************/
extern void isr_0x0a_irq();
/****************************************************************/
extern void isr_0x0b_irq();
/****************************************************************/
extern void isr_syscall();
/****************************************************************/
extern void isr_yield();

#endif
