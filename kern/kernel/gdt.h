#ifndef _GDT_H_
#define _GDT_H_

#include <kernel/types.h>

/** GDT descriptor **/

struct gdt_descriptor
{
	ui16_t limit_0_15;
	ui16_t base_0_15;
	ui8_t base_16_23;
	ui8_t access;
	ui8_t limit_16_19:4;
	ui8_t flags:4;
	ui8_t base_24_31;
} __attribute__ ((packed));

/** GDT description **/

struct gdt_description
{
	ui16_t limit;
	ui32_t base;
} __attribute__ ((packed));

/** TSS **/

struct tss
{
	ui16_t link;
	ui16_t reserved1;
  	ui32_t esp0;
  	ui16_t ss0;
	ui16_t reserved2;
	ui32_t esp1;
	ui16_t ss1;
	ui16_t reserved3;
	ui32_t esp2;
	ui16_t ss2;
	ui16_t reserved4;
	ui32_t cr3;
	ui32_t eip;
	ui32_t eflags;
	ui32_t eax;
	ui32_t ecx;
	ui32_t edx;
	ui32_t ebx;
	ui32_t esp;
	ui32_t ebp;
	ui32_t esi;
	ui32_t edi;
	ui16_t es;
	ui16_t reserved5;
	ui16_t cs;
	ui16_t reserved6;
	ui16_t ss;
	ui16_t reserved7;
	ui16_t ds;
	ui16_t reserved8;
	ui16_t fs;
	ui16_t reserved9;
	ui16_t gs;
	ui16_t reserved10;
	ui16_t ldtr;
	ui16_t reserved11;
	ui16_t debug_flag;
	ui16_t iopb_offset;
} __attribute__ ((packed));

/** Access options **/

#define GDT_ACCESS_PR		0x80
#define GDT_ACCESS_RING0	0x00
#define GDT_ACCESS_RING1	0x20
#define GDT_ACCESS_RING2	0x40
#define GDT_ACCESS_RING3	0x60
#define GDT_ACCESS_SEG		0x10
#define GDT_ACCESS_EX		0x08
#define GDT_ACCESS_DC		0x04
#define GDT_ACCESS_RW		0x02
#define GDT_ACCESS_AC		0x01

/** Flags **/

#define GDT_FLAGS_GR		0x8
#define GDT_FLAGS_SIZE_16	0x0
#define GDT_FLAGS_SIZE_32	0x4

/** Default TSS **/

#ifdef _GDT_C_
struct tss default_tss;
#else
extern struct tss default_tss;
#endif

/** Functions **/

void gdt_init();
/****************************************************************/
void gdt_init_descriptor(struct gdt_descriptor *desc,
                         ui32_t base,
                         ui32_t limit,
                         ui8_t access,
                         ui8_t flags);

#endif
