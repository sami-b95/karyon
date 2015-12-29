/****************************************************************
 * close.c                                                      *
 *                                                              *
 *    close syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/int.h>
#include <kernel/isr.h>
#include <kernel/libc.h>
#include <kernel/printk.h> // debugging
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_close
 */

int sys_close(si32_t fildes)
{
	ret_t err;

	if((err = fildes_check(fildes, 0, 0, 1, FS_NOFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	#ifdef DEBUG
	printk("close: fildes %x\n", fildes);
	printk("close unreferences inode %x\n", fd->inum);
	#endif

	fildes_unref(fildes);

	return 0;
}
