#ifndef _IO_H_
#define _IO_H_

/** out family **/

#define outb(port, value) \
	asm volatile("outb %%al, %%dx" : \
	                               : "a"(value), "d"(port))

#define outw(port, value) \
	asm volatile("outw %%ax, %%dx" : \
	                               : "a"(value), "d"(port))

#define outl(port, value) \
	asm volatile("outl %%eax, %%dx" : \
	                                : "a"(value), "d"(port))

#define outbt(port, value) \
	asm volatile("outb %%al, %%dx; jmp 1f; 1:" : \
	                                           : "a"(value), "d"(port))

#define outwt(port, value) \
	asm volatile("outw %%ax, %%dx; jmp 1f; 1:" : \
	                                           : "a"(value), "d"(port))

#define outlt(port, value) \
	asm volatile("outl %%eax, %%dx; jmp 1f; 1" : \
	                                           : "a"(value), "d"(port))

/** in family **/

#define inb(port) ({ \
	ui8_t __value; \
	asm volatile("inb %%dx, %%al" : "=a"(__value) \
	                              : "d"(port)); \
	__value; \
})

#define inw(port) ({ \
	ui16_t __value; \
	asm volatile("inw %%dx, %%ax" : "=a"(__value) \
	                              : "d"(port)); \
	__value; \
})

#define inl(port) ({ \
	ui32_t __value; \
	asm volatile("inl %%dx, %%eax" : "=a"(__value) \
	                               : "d"(port)); \
	__value; \
})

/** CMOS read/write **/

#define cmos_read(reg) ({ \
	outb(0x70, reg); \
	inb(0x71); \
})

#define cmos_write(reg, value) { \
	outb(0x70, reg); \
	outb(0x71, value); \
}

#endif
