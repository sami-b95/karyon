/****************************************************************
 * file.c                                                       *
 *                                                              *
 *    Files and file descriptors management.                    *
 *                                                              *
 ****************************************************************/

#define _FILE_C_
#include <config.h>
#ifdef ENABLE_PIPES
#include <fs/fifo.h>
#endif
#include <kernel/errno.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#ifdef ENABLE_NETWORK
#include <net/tcp.h>
#endif

#include "file.h"

/**
 * file_alloc
 */

ino_t file_alloc()
{
	ino_t inum;
	struct file *file;

	for(inum = 1; inum <= NR_FILES; inum++)
	{
		file = &file_tab[inum - 1];

		if(!file->used)
		{
			#ifdef DEBUG
			printk("inode %x is free\n", inum);
			#endif

			if(file->ref_cnt)
			{
				panic("reference count is %x instead of 0",
				      file->ref_cnt);
			}

			file->fs = FS_NOFS;
			file->used = 1;

			return inum;
		}
	}

	return 0;
}

/**
 * file_ref
 */

void file_ref(ino_t inum)
{
	if(inum < 1 || inum > NR_FILES)
	{
		panic("bad inode number %x", inum);
	}
	else if(file_tab[inum - 1].ref_cnt == 0xffffffff)
	{
		panic("cannot reference file %x anymore", inum);
	}

	#ifdef DEBUG
	printk("reference file %x (old reference count: %x)\n",
	       inum, file_tab[inum - 1].ref_cnt);
	#endif

	file_tab[inum - 1].ref_cnt++;
}

/**
 * file_unref
 */

void file_unref(ino_t inum)
{
	struct file *file = &file_tab[inum - 1];

	if(inum < 1 || inum > NR_FILES)
	{
		panic("bad inode number %x", inum);
	}
	else if(!file->ref_cnt)
	{
		panic("trying to unreference a free file (inum: %x)", inum);
	}

	file->ref_cnt--;

	if(!file->ref_cnt)
	{	
		#ifdef ENABLE_PIPES
		if(file->fs == FS_PIPEFS)
		{
			pipe_tab[file->data.pipe_id].used = 0;
		}
		#endif
		#ifdef ENABLE_NETWORK
		if(file->fs == FS_TCPSOCKFS)
		{
			struct tcp_socket *tcp_sock
			= &tcp_sock_tab[file->data.tcp_sock_id];

			tcp_sock->close_req = 1;
			tcp_socket_free(file->data.tcp_sock_id);
		}
		#endif

		file->used = 0;
	}

	#ifdef DEBUG
	printk("unref file %x (old ref count: %x)\n", inum, file->ref_cnt + 1);

	if(!file->ref_cnt)
	{
		printk("inode %x becomes free\n", inum);
	}
	#endif
}

/**
 * file_fetch
 *
 *   If the file specified by fs and key exists in the file table, this
 *   function sets *pdata to the data field of the relevant file table entry
 *   and *pinum to the relevant inum.
 *   If the file does not exist in the file table, the file is searched in the
 *   fs and the *dest is filled with the FS-specific file structure and *pdata
 *   is set to dest.
 */

ret_t file_fetch(fs_t fs, void *key, void *dest, void **pdata, ino_t *pinum)
{
	ino_t inum;
	ret_t ret;
	struct file *file;

	/** TODO: sockets. **/

	if(fs == FS_EXT2)
	{
		for(inum = 1; inum <= NR_FILES; inum++)
		{
			file = &file_tab[inum - 1];

			if(file->data.ext2_file.inum == *(ui32_t*)key
			&& file->fs == FS_EXT2
			&& file->ref_cnt)
			{
				break;
			}
		}

		if(inum > NR_FILES && dest)
		{
			if((ret = ext2_make_file(*(ui32_t*)key, dest)) != OK)
			{
				return ret;
			}
		}
	}
	else
	{
		return -EINVAL;
	}

	/** First case. **/

	if(inum <= NR_FILES)
	{
		if(pinum)
		{
			*pinum = inum;
		}

		if(pdata)
		{
			*pdata = &file->data;
		}

		return OK;
	}

	/** Second case. **/

	if(dest)
	{
		if(pinum)
		{
			*pinum = 0;
		}

		if(pdata)
		{
			*pdata = dest;
		}

		return OK;
	}

	return -EINVAL;
}

/**
 * fildes_alloc
 */

si32_t fildes_alloc(si32_t min_fildes)
{
	si32_t abs_fildes, fildes;
	struct fildes *fd;

	for(fildes = min_fildes; fildes < NR_FILDES_PER_PROC; fildes++)
	{
		if(!current->pfildes_tab[fildes])
		{
			break;
		}
	}

	if(fildes < NR_FILDES_PER_PROC)
	{
		for(abs_fildes = 0; abs_fildes < NR_FILDES; abs_fildes++)
		{
			fd = &fildes_tab[abs_fildes];

			if(!fd->used)
			{
				fd->used = 1;
				fd->ref_cnt = 1;
				fd->off = 0;
				fd->inum = 0;
				current->pfildes_tab[fildes] = fd;

				return fildes;
			}
		}
	}

	return -1;
}

/**
 * fildes_ref
 */

void fildes_ref(si32_t fildes)
{
	struct fildes *fd;
	struct file *file;
	ret_t err;

	if((err = fildes_check(fildes, &fd, &file, 1, FS_NOFS)) != OK)
	{
		panic("bad file descriptor %x", fildes);
	}

	if(fd->ref_cnt == 0xffffffff)
	{
		panic("file desc referenced too many times");
	}

	if(!file)
	{
		panic("referencing file descriptor bound to no file");
	}

	#ifdef ENABLE_PIPES
	if(file->fs == FS_PIPEFS)
	{
		struct pipe *pipe = &pipe_tab[file->data.pipe_id];

		if(current->fildes_flags[fildes] & O_WRONLY)
		{
			if(pipe->writers == 0xffffffff)
			{
				panic("cannot add pipe writers anymore");
			}

			pipe->writers++;
		}
		else
		{
			if(pipe->readers == 0xffffffff)
			{
				panic("cannot add pipe readers anymore");
			}

			pipe->readers++;
		}

		/*printk("(ref) pipe (readers, writers) = (%x, %x)\n",
		       pipe->readers, pipe->writers);*/
	}
	#endif

	file_ref(fd->inum);
	fd->ref_cnt++;
}

/**
 * fildes_unref
 */

void fildes_unref(si32_t fildes)
{
	struct fildes *fd;
	struct file *file;
	ret_t err;

	if((err = fildes_check(fildes, &fd, &file, 1, FS_NOFS)) != OK)
	{
		panic("trying to unreference invalid fildes (error %x)",
		      err);
	}

	#ifdef DEBUG
	printk("fildes_unref unreferences inode %x\n", fd->inum);
	#endif

	if(file)
	{
		#ifdef ENABLE_PIPES
		if(file->fs == FS_PIPEFS)
		{
			struct pipe *pipe = &pipe_tab[file->data.pipe_id];

			if(current->fildes_flags[fildes] & O_WRONLY)
			{
				if(!pipe->writers)
				{
					panic("pipe writers count corrupted");
				}

				pipe->writers--;
			}
			else
			{
				if(!pipe->readers)
				{
					panic("pipe readers count corrupted");
				}

				pipe->readers--;
			}

			/*printk("(unref) pipe (readers, writers) = (%x, %x)\n",
			       pipe->readers, pipe->writers);*/
		}
		#endif

		file_unref(fd->inum);
	}

	current->pfildes_tab[fildes] = 0;

	if(!fd->ref_cnt)
	{
		panic("trying to unreference unreferenced file descriptor");
	}

	fd->ref_cnt--;

	if(!fd->ref_cnt)
	{
		fd->used = 0;
	}
}

/**
 * fildes_check
 */

ret_t fildes_check(si32_t fildes,
                   struct fildes **fd,
                   struct file **file,
                   bool_t usage,
                   fs_t fs)
{
	struct fildes *_fd = 0;
	struct file *_file = 0;

	if(fildes < 0 || fildes >= NR_FILDES_PER_PROC)
	{
		return -EBADF;
	}

	if(usage)
	{
		/** Check for usage. **/

		_fd = current->pfildes_tab[fildes];

		if(!_fd)
		{
			return -EBADF;
		}

		_file = _fd->inum ? &file_tab[_fd->inum - 1] : 0;

		/** Check for file system. **/

		if(fs != FS_NOFS && (!_file || _file->fs != fs))
		{
			if(fs == FS_TCPSOCKFS)
			{
				return -ENOTSOCK;
			}

			return -EBADF;
		}
	}

	if(fd)
	{
		*fd = _fd;
	}

	if(file)
	{
		*file = _file;
	}

	return OK;
}
