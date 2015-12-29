/****************************************************************
 * tcgpgrp.c                                                    *
 *                                                              *
 *    tcgetpgrp syscall.                                        *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <fs/file.h>
#include <fs/tty.h>
#include <kernel/errno.h>
#ifdef DEBUG
#include <kernel/printk.h>
#endif
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_tcgetpgrp
 */

pid_t sys_tcgetpgrp(si32_t fildes)
{
	struct fildes *fd;
	ret_t err;

	if((err = fildes_check(fildes, &fd, 0, 1, FS_DEVFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	/** Check whether the file represents the console. **/

	if(fd->inum != 1)
	{
		#ifdef DEBUG
		printk("error 3\n");
		#endif
		return -1;
	}

	/** Return the foreground process group. **/

	#ifdef DEBUG
	printk("pgrp: %x\n", pgrp);
	#endif

	return pgrp;
}
