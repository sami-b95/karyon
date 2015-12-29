/****************************************************************
 * gsockopt.c                                                   *
 *                                                              *
 *    getsockopt syscall.                                       *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/file.h>
#include <kernel/printk.h>
#include <kernel/types.h>

/**
 * sys_getsockopt
 */

int sys_getsockopt(si32_t fildes,
                   ui32_t level,
                   ui32_t opt_name,
                   void *opt_val,
                   socklen_t *opt_len)
{
	#ifdef DEBUG
	printk("sys_getsockopt(%x, %x, %x, %x, %x)\n",
	       fildes, level, opt_name, opt_val, opt_len);
	#endif

	return -1;
}
