/****************************************************************
 * printk.c                                                     *
 *                                                              *
 *    printk function.                                          *
 *                                                              *
 ****************************************************************/

#include <kernel/libc.h>
#include <kernel/screen.h>
#include <kernel/stdarg.h>

#include "printk.h"

/**
 * printk
 */

size_t printk(uchar_t *f, ...)
{
	size_t ret;
	uchar_t s[256];

	va_list ap;
	va_start(ap, f);
	ret = vsnprintf(s, 255, f, ap);
	va_end(ap);

	screen_print(s);

	return ret;
}
