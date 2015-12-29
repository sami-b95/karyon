/****************************************************************
 * lseek.c                                                      *
 *                                                              *
 *    lseek syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/process.h>
#include <kernel/printk.h> // debug
#include <kernel/types.h>

/**
 * sys_lseek
 */

off_t sys_lseek(si32_t fildes, off_t off, ui32_t whence)
{
	struct fildes *fd;
	struct file *file;
	ret_t err;

	if((err = fildes_check(fildes, &fd, &file, 1, FS_NOFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	/** If the file is not a regular file and is not the disk file, return
	    an error. **/

	if(file->fs != FS_EXT2 && fd->inum != 2)
	{
		return -1;
	}

	/** Try to do the seek. **/

	if(whence == SEEK_SET)
	{
		fd->off = off;
	}
	else if(whence == SEEK_CUR)
	{
		fd->off += off;
	}
	else if(whence == SEEK_END)
	{
		if(file->fs != FS_EXT2)
		{
			return -1;
		}
		else
		{
			#ifdef DEBUG
			printk("file size: %x\n",
			       file->data.ext2_file.inode.i_size_low);
			#endif
			fd->off = (off_t)file->data.ext2_file.inode.i_size_low
			        + off;
		}
	}
	else
	{
		return -1;
	}

	return fd->off;
}
