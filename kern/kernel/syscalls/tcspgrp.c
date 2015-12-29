/****************************************************************
 * tcspgrp.c                                                    *
 *                                                              *
 *    tcsetpgrp syscall.                                        *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <fs/file.h>
#include <fs/tty.h>
#include <kernel/errno.h>
#ifdef DEBUG
#include <kernel/libc.h>
#include <kernel/printk.h>
#endif
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_tcsetpgrp
 */

pid_t sys_tcsetpgrp(si32_t fildes, pid_t pg)
{
	struct fildes *fd;
	ret_t err;

	#ifdef DEBUG
	printk("tcsetpgrp(%x, %x)\n", fildes, pg);
	delay(20000);
	#endif

	if((err = fildes_check(fildes, &fd, 0, 1, FS_DEVFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	/** Check whether the file represents the console. **/

	if(fd->inum != 1)
	{
		return -1;
	}

	/** Set the foreground process group. **/

	pgrp = pg;

	return 0;
}
