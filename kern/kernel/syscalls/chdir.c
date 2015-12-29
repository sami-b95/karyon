/****************************************************************
 * chdir.c                                                      *
 *                                                              *
 *    chdir syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/ext2.h>
#include <fs/path.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_chdir
 */

int sys_chdir(uchar_t *path)
{
	ui32_t file_inum;
	struct ext2_file file;
	struct ext2_file *pfile;

	/** Check whether the path is not too long. **/

	if(strlen(path) > PATH_MAX_LEN)
	{
		return -1;
	}

	/** Clean the path. **/

	strncpy(tmp_path, path, PATH_MAX_LEN + 1);

	if(path_clean(tmp_path) != OK)
	{
		return -1;
	}

	/** Check whether the path exists. **/

	if(path_to_file(tmp_path, &file_inum, 0, 0) != OK)
	{
		return -1;
	}

	if(file_fetch(FS_EXT2, &file_inum, &file, (void**)&pfile, 0) != OK)
	{
		return -1;
	}

	/** Check whether the file a directory. **/

	if(!(pfile->inode.i_type_perm & EXT2_DIR))
	{
		return -1;
	}

	/** Set the current working directory. **/

	strncpy(current->cwd, tmp_path, PATH_MAX_LEN + 1);

	return 0;
}
