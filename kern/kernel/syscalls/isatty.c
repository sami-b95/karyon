/****************************************************************
 * isatty.c                                                     *
 *                                                              *
 *    isatty syscall.                                           *
 *                                                              *
 ****************************************************************/

#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_isatty
 */

int sys_isatty(si32_t fildes)
{
	struct fildes *fd;
	ret_t err;

	if((err = fildes_check(fildes, &fd, 0, 1, FS_DEVFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return 0;
	}

	return (fd->inum == 1);
}
