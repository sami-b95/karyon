/****************************************************************
 * idt.c                                                        *
 *                                                              *
 *    IDT initialization.                                       *
 *                                                              *
 ****************************************************************/

#include <kernel/libc.h>
#include <mm/mem_map.h>

#include "idt.h"

struct idt_descriptor idt_desc[256] = {
	{
		.offset_0_15 = 0,
		.selector = 0,
		.zero = 0,
		.type = 0,
		.attr = 0,
		.offset_16_31 = 0
	}
};
struct idt_description idt_descript = {
	.limit = 256 * sizeof(struct idt_descriptor) - 1,
	.base = IDT_BASE
};

/**
 * idt_init
 */

void idt_init()
{
	ui8_t desc;

	/** Setup interrupt descriptors **/

	/*** Exceptions ***/

	for(desc = 0; desc < 17; desc++)
	{
		idt_init_descriptor(&idt_desc[desc],
		                    (ui32_t)isr_default_exc,
		                    IDT_TYPE_32_INT_GATE,
		                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING0,
		                    0x08);
	}

	idt_init_descriptor(&idt_desc[0xe],
	                    (ui32_t)isr_pf_exc,
	                    IDT_TYPE_32_INT_GATE,
	                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING0,
	                    0x08);

	/*** IRQs ***/

	for(desc = 0x20; desc < 0x28; desc++)
	{
		idt_init_descriptor(&idt_desc[desc],
		                    (ui32_t)isr_default_pic1_irq,
		                    IDT_TYPE_32_INT_GATE,
		                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING0,
		                    0x08);
	}

	for(desc = 0x70; desc < 0x78; desc++)
	{
		idt_init_descriptor(&idt_desc[desc],
		                    (ui32_t)isr_default_pic2_irq,
		                    IDT_TYPE_32_INT_GATE,
		                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING0,
		                    0x08);
	}

	idt_init_descriptor(&idt_desc[0x20],
	                    (ui32_t)isr_pit_irq,
	                    IDT_TYPE_32_INT_GATE,
	                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING0,
	                    0x08);

	idt_init_descriptor(&idt_desc[0x21],
	                    (ui32_t)isr_kbd_irq,
	                    IDT_TYPE_32_INT_GATE,
	                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING0,
	                    0x08);

	idt_init_descriptor(&idt_desc[0x71],
	                    (ui32_t)isr_0x09_irq,
	                    IDT_TYPE_32_INT_GATE,
	                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING0,
	                    0x08);

	idt_init_descriptor(&idt_desc[0x72],
	                    (ui32_t)isr_0x0a_irq,
	                    IDT_TYPE_32_INT_GATE,
	                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING0,
	                    0x08);

	idt_init_descriptor(&idt_desc[0x73],
	                    (ui32_t)isr_0x0b_irq,
	                    IDT_TYPE_32_INT_GATE,
	                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING0,
	                    0x08);

	/*** System call ***/

	idt_init_descriptor(&idt_desc[0x30],
	                    (ui32_t)isr_syscall,
	                    IDT_TYPE_32_TRAP_GATE,
	                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING3,
	                    0x08);

	/** Yield hack. **/

	idt_init_descriptor(&idt_desc[0x80],
	                    (ui32_t)isr_yield,
	                    IDT_TYPE_32_TRAP_GATE,
	                    IDT_ATTRIBUTE_P | IDT_ATTRIBUTE_RING3,
	                    0x08);

	/** Load IDT **/

	memcpy((void*)IDT_BASE,
	       &idt_desc,
	       256 * sizeof(struct idt_descriptor));

	asm volatile("lidt (idt_descript)");
}

/**
 * idt_init_descriptor
 */

void idt_init_descriptor(struct idt_descriptor *desc,
                         ui32_t offset,
                         ui8_t type,
                         ui8_t attr,
                         ui16_t selector)
{
	desc->offset_0_15 = offset & 0xffff;
	desc->offset_16_31 = offset >> 16;
	desc->type = type;
	desc->attr = attr;
	desc->selector = selector;
	desc->zero = 0;
}
