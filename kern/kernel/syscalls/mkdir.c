/****************************************************************
 * mkdir.c                                                      *
 *                                                              *
 *    mkdir syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <fs/ext2.h>
#include <fs/path.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_mkdir
 */

int sys_mkdir(uchar_t *path, mode_t mode)
{
	struct ext2_file file;

	/** Check whether the path is not too long. **/

	if(strlen(path) > PATH_MAX_LEN)
	{
		return -1;
	}

	/** Try to clean the path. **/

	strncpy(tmp_path, path, PATH_MAX_LEN + 1);

	if(path_clean(tmp_path) != OK)
	{
		return -1;
	}

	/** If the directory exists, abort. **/

	if(path_to_file(tmp_path, 0, 0, 0) == OK)
	{
		*current->perrno = EEXIST;
		return -1;
	}

	/** Try to create the directory. **/

	if(path_create(tmp_path, 1, 0, &file) != OK)
	{
		return -1;
	}

	return 0;
}
