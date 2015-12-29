/****************************************************************
 * gdt.c                                                        *
 *                                                              *
 *    GDT initialization.                                       *
 *                                                              *
 ****************************************************************/

#define _GDT_C_
#include <kernel/libc.h>
#include <mm/mem_map.h>

#include "gdt.h"

struct gdt_descriptor gdt_desc[8];
struct gdt_description gdt_descript = {
	.limit = 8 * sizeof(struct gdt_descriptor),
	/* NOTE: DO NOT ADD -1 to limit (contrary to the case of the IDT). It
	         made my real machine crash (while neither qemu nor Bochs did)
	         and it took me a long time to work out the origin of the
	         error. */
	.base = GDT_BASE
};

/**
 * gdt_init
 */

void gdt_init()
{
	/** Setup segment descriptors **/

	/*** Null segment ***/

	gdt_init_descriptor(&gdt_desc[0], 0, 0, 0, 0);

	/*** Kernel segments ***/

	gdt_init_descriptor(&gdt_desc[1], 0, 0xfffff,
	                    GDT_ACCESS_PR
	                  | GDT_ACCESS_RING0
	                  | GDT_ACCESS_SEG
	                  | GDT_ACCESS_EX
	                  | GDT_ACCESS_RW
	                  | GDT_ACCESS_AC,
	                    GDT_FLAGS_GR
	                  | GDT_FLAGS_SIZE_32);

	gdt_init_descriptor(&gdt_desc[2], 0, 0xfffff,
	                    GDT_ACCESS_PR
	                  | GDT_ACCESS_RING0
	                  | GDT_ACCESS_SEG
	                  | GDT_ACCESS_RW
	                  | GDT_ACCESS_AC,
	                    GDT_FLAGS_GR
	                  | GDT_FLAGS_SIZE_32);

	gdt_init_descriptor(&gdt_desc[3], 0, 0,
	                    GDT_ACCESS_PR
	                  | GDT_ACCESS_RING0
	                  | GDT_ACCESS_SEG
	                  | GDT_ACCESS_DC
	                  | GDT_ACCESS_RW
	                  | GDT_ACCESS_AC,
	                    GDT_FLAGS_GR
	                  | GDT_FLAGS_SIZE_32);

	/*** User segments ***/

	gdt_init_descriptor(&gdt_desc[4], 0, 0xfffff,
	                    GDT_ACCESS_PR
	                  | GDT_ACCESS_RING3
	                  | GDT_ACCESS_SEG
	                  | GDT_ACCESS_EX
	                  | GDT_ACCESS_DC // NOTE: is this one useful ?
	                  | GDT_ACCESS_RW
	                  | GDT_ACCESS_AC,
	                    GDT_FLAGS_GR
	                  | GDT_FLAGS_SIZE_32);

	gdt_init_descriptor(&gdt_desc[5], 0, 0xfffff,
	                    GDT_ACCESS_PR
	                  | GDT_ACCESS_RING3
	                  | GDT_ACCESS_SEG
	                  | GDT_ACCESS_RW
	                  | GDT_ACCESS_AC,
	                    GDT_FLAGS_GR
	                  | GDT_FLAGS_SIZE_32);

	gdt_init_descriptor(&gdt_desc[6], 0, 0,
	                    GDT_ACCESS_PR
	                  | GDT_ACCESS_RING3
	                  | GDT_ACCESS_SEG
	                  | GDT_ACCESS_DC
	                  | GDT_ACCESS_RW
	                  | GDT_ACCESS_AC,
	                    GDT_FLAGS_GR
	                  | GDT_FLAGS_SIZE_32);

	/*** TSS ***/

	default_tss.debug_flag = 0;
	default_tss.iopb_offset = 0;
	default_tss.esp0 = 0x6ffc;
	default_tss.ss0 = 0x18;

	gdt_init_descriptor(&gdt_desc[7],
	                    (ui32_t)&default_tss,
	                    sizeof(struct tss) - 1,
	                    GDT_ACCESS_PR
	                  | GDT_ACCESS_RING3
	                  | GDT_ACCESS_EX
	                  | GDT_ACCESS_AC,
	                    0);

	/** Load GDT **/

	memcpy((void*)GDT_BASE,
	       &gdt_desc,
	       8 * sizeof(struct gdt_descriptor));

	asm volatile("lgdt (gdt_descript) \n\
	              movw $0x10, %%ax \n\
	              movw %%ax, %%ds \n\
	              movw %%ax, %%es \n\
	              movw %%ax, %%fs \n\
	              movw %%ax, %%gs \n\
	              ljmp $0x08, $next \n\
	              next:" ::: "eax");
}

/**
 * gdt_init_descriptor
 */

void gdt_init_descriptor(struct gdt_descriptor *desc,
                         ui32_t base,
                         ui32_t limit,
                         ui8_t access,
                         ui8_t flags)
{
	desc->base_0_15 = base & 0xffff;
	desc->base_16_23 = (base >> 16) & 0xff;
	desc->base_24_31 = base >> 24;
	desc->limit_0_15 = limit & 0xffff;
	desc->limit_16_19 = (limit >> 16) & 0xf;
	desc->access = access;
	desc->flags = flags;
}
