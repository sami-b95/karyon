/****************************************************************
 * write.c                                                      *
 *                                                              *
 *    write syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <fs/disk.h>
#include <fs/file.h>
#include <fs/tty.h>
#include <kernel/errno.h>
#include <kernel/int.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/screen.h>
#include <kernel/syscall.h>
#include <kernel/types.h>

/**
 * sys_write
 */

ssize_t sys_write(si32_t fildes, void *buf, size_t size)
{
	struct fildes *fd;
	struct file *file;
	size_t to_append;
	ssize_t ret;
	ret_t err;

	#ifdef DEBUG
	printk("write(%x, %x, %x)\r\n", fildes, (ui32_t)buf, size);
	delay(1000);
	#endif

	/** Check whether the file descriptor is valid. **/

	if((err = fildes_check(fildes, &fd, &file, 1, FS_NOFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	/** Check permissions. **/

	if(!(current->fildes_flags[fildes] & O_WRONLY)
	&& !(current->fildes_flags[fildes] & O_RDWR))
	{
		return -1;
	}

	/** Check the file. **/

	if(!file->ref_cnt)
	{
		panic("file descriptor refers to a file with null ref cnt");
		/*#ifdef DEBUG
		printk("sys_write: null reference count\n");
		#endif
		return -1;*/
	}

	/** Try to write to the file. **/

	if(file->fs == FS_EXT2)
	{
		struct ext2_inode *inode = &file->data.ext2_file.inode;

		/** Do not allow writing to directories. **/

		if(inode->i_type_perm & EXT2_DIR)
		{
			return -1;
		}

		/** Append bytes to the file if required. **/

		if((fd->off + size) > inode->i_size_low)
		{
			to_append = fd->off + size - inode->i_size_low;

			#ifdef DEBUG
			printk("appending %x bytes to file\n", to_append);
			#endif

			if(ext2_append(&file->data.ext2_file,
			               to_append) != OK)
			{
				return -1;
			}
		}

		/** Write to the file. **/

		ret = ext2_write(&file->data.ext2_file,
		                 buf,
		                 size,
		                 fd->off);

		if(ret == -1)
		{
			#ifdef DEBUG
			printk("ext2_write failed for %x\n",
			       file->data.ext2_file.inum);
			#endif
			return -1;
		}

		fd->off += ret;

		return ret;
	}
	#ifdef ENABLE_NETWORK
	else if(file->fs == FS_TCPSOCKFS)
	{
		return sys_send(fildes, buf, size, 0);
	}
	#endif
	#ifdef ENABLE_PIPES
	else if(file->fs == FS_PIPEFS)
	{
		struct pipe *pipe = &pipe_tab[file->data.pipe_id];

		ret = 0;

		do
		{
			sti;
			yield;
			//delay(1);
			cli;

			if(!pipe->readers)
			{
				*current->perrno = EPIPE;
				current->sigset |= (1 << (SIGPIPE - 1));
				return -1;
			}

			if(size <= PIPE_ATOMIC_LIMIT)
			{
				if(fifo_left(&pipe->fifo) >= size)
				{
					if(fifo_write(&pipe->fifo,
					              buf,
					              size,
					              0) != OK)
					{
						panic("pipe corrupted");
					}

					//printk("wrote %s(%x)\n", buf, size);
					ret = size;
				}
				else if(current->fildes_flags[fildes]
				      & O_NONBLOCK)
				{
					*current->perrno = EWOULDBLOCK;
					ret = -1;
				}
			}
			else
			{
				while(fifo_write_byte(&pipe->fifo,
				                      *(ui8_t*)(buf + ret),
				                      0) == OK
				   && ret < size)
				{
					ret++;
				}

				if((current->fildes_flags[fildes] & O_NONBLOCK)
				&& ret < size)
				{
					*current->perrno = EWOULDBLOCK;
				}
			}

			//printk("wrote %x/%x to pipe\n", ret, size);
		} while(!(current->fildes_flags[fildes] & O_NONBLOCK)
		        && ret < size);

		return ret;
	}
	#endif
	else if(fd->inum == 1)
	{
		/*#ifdef DEBUG
		printk("writing to terminal\n");
		#endif*/

		if(file->fs != FS_DEVFS)
		{
			panic("inode 1 should be that of the tty");
		}

		/** Write characters to the terminal. **/

		return tty_puts(buf, size);
	}
	else if(fd->inum == 2)
	{
		if(disk_write(buf, size, fd->off) != OK)
		{
			ret = -1;
		}
		else
		{
			ret = (ssize_t)size;
			fd->off += size;
		}

		return ret;
	}
	else
	{
		panic("opened file on unrecognized file system");
		return -1;
	}
}
