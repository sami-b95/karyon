/****************************************************************
 * link.c                                                       *
 *                                                              *
 *    link syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/ext2.h>
#include <fs/path.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/types.h>

/**
 * sys_link
 */

int sys_link(uchar_t *old_path, uchar_t *new_path)
{
	ui32_t old_file_inum, new_file_inum;
	struct ext2_file new_file;

	// WARNING: stack overflow

	if(strlen(old_path) > PATH_MAX_LEN
	|| strlen(new_path) > PATH_MAX_LEN)
	{
		return -1;
	}

	/** Look for the old file. **/

	strncpy(tmp_path, old_path, PATH_MAX_LEN + 1);

	if(path_clean(tmp_path) != OK)
	{
		return -1;
	}

	if(path_to_file(tmp_path, &old_file_inum, 0, 0) != OK)
	{
		return -1;
	}

	/** Look for the new file. If it exists, return an error. **/

	strncpy(tmp_path, new_path, PATH_MAX_LEN + 1);

	if(path_clean(tmp_path) != OK)
	{
		return -1;
	}

	if(path_to_file(tmp_path, &new_file_inum, 0, 0) == OK)
	{
		return -1;
	}

	/** Create the new file. **/

	if(path_create(tmp_path, 0, old_file_inum, &new_file) != OK)
	{
		return -1;
	}

	return 0;
}
