/****************************************************************
 * unlink.c                                                     *
 *                                                              *
 *    unlink syscall.                                           *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/ext2.h>
#include <fs/file.h>
#include <fs/path.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_unlink
 */

int sys_unlink(uchar_t *path)
{
	ui32_t file_inum, dir_inum;
	struct ext2_file file, dir;
	struct ext2_file *pfile, *pdir;
	uchar_t file_name[NAME_MAX_LEN + 1];
	ret_t ret;

	/** Look for the file on the disk. **/

	if(strlen(tmp_path) > PATH_MAX_LEN)
	{
		return -1;
	}

	strncpy(tmp_path, path, PATH_MAX_LEN + 1);

	if(path_clean(tmp_path) != OK)
	{
		return -1;
	}

	if(path_to_file(tmp_path, &file_inum, &dir_inum, file_name) != OK)
	{
		return -1;
	}

	if((ret = file_fetch(FS_EXT2,
	                     &file_inum,
	                     &file,
	                     (void**)&pfile,
	                     0)) != OK
	|| (ret = file_fetch(FS_EXT2,
	                     &dir_inum,
	                     &dir,
	                     (void**)&pdir,
	                     0)) != OK)
	{
		return -1;
	}

	/** If the file the link refers to is in use and this link is the last
	    one, abort. **/

	if(pfile != &file && pfile->inode.i_links == 1)
	{
		*current->perrno = EBUSY;
		return -1;
	}

	/** Unlink the file. **/

	if(ext2_unlink(pdir, pfile, file_name) != OK)
	{
		return -1;
	}

	return 0;
}
