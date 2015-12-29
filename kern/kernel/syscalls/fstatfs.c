/****************************************************************
 * fstatfs.c                                                    *
 *                                                              *
 *    fstatfs syscall.                                          *
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
 * sys_fstatfs
 */

int sys_fstatfs(si32_t fildes, struct statfs *st)
{
	struct fildes *fd;
	struct file *file;
	ret_t err;

	#ifdef DEBUG
	printk("fstatfs(%x, %x)\n", fildes, (ui32_t)st);
	#endif

	if((err = fildes_check(fildes, &fd, &file, 1, FS_NOFS)) != OK)
	{
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
		st->f_type = 0xef53;
		st->f_bsize = 1024;
		st->f_blocks = (fsblkcnt_t)sb.s_blocks;
		st->f_bavail = st->f_bfree = (fsblkcnt_t)sb.s_free_blocks;
		st->f_files = (fsfilcnt_t)sb.s_inodes;
		st->f_ffree = (fsfilcnt_t)sb.s_free_inodes;
		st->f_fsid.val[0] = st->f_fsid.val[1] = 0;
		st->f_namelen = 256;
		st->f_frsize = 1024;
	}
	else
	{
		*current->perrno = ENOSYS;
		return -1;
	}

	return 0;
}
