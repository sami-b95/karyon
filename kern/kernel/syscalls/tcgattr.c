/****************************************************************
 * tcgattr.c                                                    *
 *                                                              *
 *    tcgetattr syscall.                                        *
 *                                                              *
 ****************************************************************/

#include <fs/file.h>
#include <fs/tty.h>
#include <kernel/errno.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_tcgetattr
 */

int sys_tcgetattr(si32_t fildes, struct termios *termios_p)
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

	/** Get the terminal attribute. **/

	*termios_p = termios;

	return 0;
}
