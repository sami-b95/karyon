#ifndef _PANIC_H_
#define _PANIC_H_

#include <kernel/int.h>
#include <kernel/printk.h>

/** panic macro **/

#define panic(...) { \
	cli; \
	printk("%s: %s: panic: ", __FILE__, __func__); \
	printk(__VA_ARGS__); \
	hlt; \
}

#endif
