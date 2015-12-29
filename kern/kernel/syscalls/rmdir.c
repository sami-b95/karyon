/****************************************************************
 * rmdir.c                                                      *
 *                                                              *
 *    rmdir syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <fs/ext2.h>
#include <fs/file.h>
#include <fs/path.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_rmdir
 */

int sys_rmdir(uchar_t *path)
{
	ui32_t dir_inum, parent_dir_inum;
	struct ext2_file dir, parent_dir;
	struct ext2_file *pdir, *pparent_dir;
	uchar_t dir_name[NAME_MAX_LEN + 1];
	ret_t ret;

	// WARNING: stack overflow

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

	/** Look for the directory. **/

	if(path_to_file(tmp_path, &dir_inum, &parent_dir_inum, dir_name) != OK)
	{
		return -1;
	}

	if((ret = file_fetch(FS_EXT2, &dir_inum, &dir, (void**)&pdir, 0)) != OK
	|| (ret = file_fetch(FS_EXT2,
	                     &parent_dir_inum,
	                     &parent_dir,
	                     (void**)&pparent_dir,
	                     0)) != OK)
	{
		return -1;
	}

	/** Check whether the file is a directory. **/

	if(!(pdir->inode.i_type_perm & EXT2_DIR))
	{
		*current->perrno = ENOTDIR;
		return -1;
	}

	/** Check whether the directory can be removed. **/

	if(/*pdir->inode.i_links == 1 && */pdir != &dir) // WARNING
	{
		*current->perrno = EBUSY;
		return -1;
	}
	else if(!ext2_dir_empty(dir_inum))
	{
		*current->perrno = ENOTEMPTY;
		return -1;
	}

	/** Do not forget to unlink '.' and '..'. **/

	if((ret = ext2_unlink(pdir, pdir, ".")) != OK
	|| (ret = ext2_unlink(pdir, pparent_dir, "..")) != OK
	|| (ret = ext2_unlink(pparent_dir, pdir, dir_name)) != OK)
	{
		return ret;
	}

	return 0;
}
