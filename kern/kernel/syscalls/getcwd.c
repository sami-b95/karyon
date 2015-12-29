/****************************************************************
 * getcwd.c                                                     *
 *                                                              *
 *    getcwd syscall.                                           *
 *                                                              *
 ****************************************************************/

#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_getcwd
 */

uchar_t *sys_getcwd(uchar_t *buf, size_t size)
{
	uchar_t *cwd = current->cwd;

	if(!buf)
	{
		panic("buf is null");
	}

	/** Check whether the current working directory is not too long. **/

	if(strlen(cwd) >= size)
	{
		return 0;
	}

	/** Copy the current working directory to the buffer. **/

	strncpy(buf, cwd, size);

	return buf;
}
