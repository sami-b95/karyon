/****************************************************************
 * tcflush.c                                                    *
 *                                                              *
 *    tcflush syscall.                                          *
 *                                                              *
 ****************************************************************/

#include <fs/file.h>
#include <fs/tty.h>
#include <kernel/errno.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_tcflush
 */

int sys_tcflush(si32_t fildes, ui32_t queue_selector)
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
		return -1;
	}

	/** Flush the FIFOs if required. **/

	if(queue_selector == TCIFLUSH || queue_selector == TCIOFLUSH)
	{
		fifo_flush(&tty_ififo);
		fifo_flush(&tty_ififo2);
	}

	return 0;
}
