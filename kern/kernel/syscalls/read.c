/****************************************************************
 * read.c                                                       *
 *                                                              *
 *    read syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <fs/disk.h>
#include <fs/ext2.h>
#include <fs/fifo.h>
#include <fs/file.h>
#include <fs/tty.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/syscall.h>
#include <kernel/types.h>

/**
 * sys_read
 */

ssize_t sys_read(si32_t fildes, void *buf, size_t size)
{
	struct fildes *fd;
	struct file *file;
	ssize_t ret;
	ret_t err;

	#ifdef DEBUG
	printk("read(%x, %x, %x)\r\n", fildes, (ui32_t)buf, size);
	#endif

	/** Check whether the file descriptor is valid. **/

	if((err = fildes_check(fildes, &fd, &file, 1, FS_NOFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	/** Check whether the file is valid. **/

	if(!file->ref_cnt)
	{
		panic("file descriptor refers to a file with null ref cnt");
	}

	/** Check buf validity. **/

	/*if(!in_user_range(buf, buf + size))
	{
		*current->perrno = EFAULT;
		return -1;
	}*/

	/** Try to read the file. **/

	if(file->fs == FS_EXT2)
	{
		struct ext2_inode *inode = &file->data.ext2_file.inode;

		if(fd->off == inode->i_size_low)
		{
			ret = 0;
			goto end;
		}
		else if(fd->off > inode->i_size_low)
		{
			ret = -1;
			goto end;
		}
		else if((fd->off + size) > inode->i_size_low)
		{
			size = inode->i_size_low - fd->off;
		}

		ret = ext2_read(file->data.ext2_file.inum, buf, size, fd->off);

		if(ret == -1)
		{
			ret = -1;
			goto end;
		}

		fd->off += ret;
	}
	else if(file->fs == FS_DEVFS)
	{
		if(fd->inum == 1)
		{
			ret = tty_read(buf, size);
		}
		else if(fd->inum == 2)
		{
			if(disk_read(buf, size, fd->off) != OK)
			{
				ret = -1;
			}
			else
			{
				ret = (ssize_t)size;
				fd->off += size;
			}
		}
		else
		{
			ret = -1;
		}
	}
	#ifdef ENABLE_NETWORK
	else if(file->fs == FS_TCPSOCKFS)
	{
		ret = sys_recv(fildes, buf, size, 0);
	}
	#endif
	#ifdef ENABLE_PIPES
	else if(file->fs == FS_PIPEFS)
	{
		struct pipe *pipe = &pipe_tab[file->data.pipe_id];

		ret = 0;

		while(pipe->writers || pipe->fifo.to_read)
		{
			ret = (ssize_t)fifo_read(&pipe->fifo, buf, size, 0);

			if(ret || current->fildes_flags[fildes] & O_NONBLOCK)
			{
				break;
			}

			sti;
			yield;
			cli;
		}

		/*printk("read bytes \"%s\"(%x) to %x from pipe (pid: %x)\n",
		       buf, ret, buf, current_pid);
		delay(1200000);*/

		/*if(!ret)
		{
			while(1);
		}*/
	}
	#endif
	else
	{
		panic("opened file on unrecognized file system");
	}

	end:
		return ret;
}
