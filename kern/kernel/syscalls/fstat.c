/****************************************************************
 * fstat.c                                                      *
 *                                                              *
 *    fstat syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/ext2.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/panic.h>
#include <kernel/printk.h> // for debugging
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_fstat
 */

int sys_fstat(si32_t fildes, struct stat *st)
{
	struct fildes *fd;
	struct file *file;
	ret_t err;

	#ifdef DEBUG
	printk("fstat(%x, %x)\n", fildes, (ui32_t)st);
	#endif

	if((err = fildes_check(fildes, &fd, &file, 1, FS_NOFS)) != OK)
	{
		cli;
		hlt;
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	/** Check st address. **/

	if(!in_user_range(st, st + 1))
	{
		*current->perrno = EFAULT;
		return -1;
	}

	/** Fill the stat structure. **/

	if(file->fs == FS_EXT2)
	{
		if(file->data.ext2_file.inode.i_type_perm & EXT2_DIR)
		{
			st->st_mode = S_IFDIR;
		}
		else
		{
			st->st_mode = S_IFREG;
		}

		st->st_dev = 2;
		st->st_rdev = 0;
		st->st_size = (off_t)file->data.ext2_file.inode.i_size_low;
	}
	else if(fd->inum == 2)
	{
		if(file->fs != FS_DEVFS)
		{
			panic("inode 2 should be that of the disk");
		}

		st->st_mode = S_IFBLK;
		st->st_dev = 0;
		st->st_rdev = 2;
		st->st_size = DISK_SIZE;
	}
	else if(fd->inum == 1)
	{
		if(file->fs != FS_DEVFS)
		{
			panic("inode 1 should be that of the tty");
		}

		st->st_mode = S_IFCHR;
		st->st_dev = 0;
		st->st_rdev = 1;
		st->st_size = 0;
	}

	st->st_ino = fd->inum;
	st->st_mode |= 0777;
	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_blksize = 512;
	st->st_blocks = (st->st_size + 511) / 512;

	return 0;
}
