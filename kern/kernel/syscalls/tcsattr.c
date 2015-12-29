/****************************************************************
 * tcsattr.c                                                    *
 *                                                              *
 *    tcsetattr syscall.                                        *
 *                                                              *
 ****************************************************************/

#include <fs/fifo.h>
#include <fs/file.h>
#include <fs/tty.h>
#include <kernel/errno.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_tcsetattr
 */

int sys_tcsetattr(si32_t fildes, ui32_t opt_act, struct termios *termios_p)
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

	/** Set the terminal attribute. **/

	termios = *termios_p;

	if(opt_act == TCSAFLUSH)
	{
		fifo_flush(&tty_ififo);
		fifo_flush(&tty_ififo2);
	}

	return 0;
}
