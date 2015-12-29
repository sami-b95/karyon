#ifndef _INT_H_
#define _INT_H_

#define cli	asm volatile("cli")
#define sti	asm volatile("sti")
#define hlt	asm volatile("hlt")
#define yield	asm volatile("int $0x80")

#endif
