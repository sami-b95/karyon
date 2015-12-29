/****************************************************************
 * ftruncate.c                                                  *
 *                                                              *
 *    ftruncate syscall.                                        *
 *                                                              *
 ****************************************************************/

#include <fs/ext2.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_ftruncate
 */

int sys_ftruncate(si32_t fildes, off_t length)
{
	struct fildes *fd;
	struct file *file;
	struct ext2_inode *inode;
	ret_t err;

	#ifdef DEBUG
	printk("ftruncate(%x, %x)\r\n", fildes, length);
	#endif

	/** Check whether the file descriptor is valid. **/

	if((err = fildes_check(fildes, &fd, &file, 1, FS_NOFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	/** Truncate/enlarge the file when possible. **/

	*current->perrno = 0;

	if(file->fs != FS_EXT2)
	{
		*current->perrno = EPERM;
		return -1;
	}

	inode = &file->data.ext2_file.inode;

	if(!(inode->i_type_perm & EXT2_REG_FILE))
	{
		*current->perrno = EISDIR;
		return -1;
	}

	if(inode->i_size_low > (ui32_t)length)
	{
		*current->perrno = -ext2_truncate(&file->data.ext2_file,
		                                  length);
	}
	else
	{
		*current->perrno = -ext2_append(&file->data.ext2_file,
		                                length - inode->i_size_low);
	}

	return *current->perrno ? -1 : 0;
}
