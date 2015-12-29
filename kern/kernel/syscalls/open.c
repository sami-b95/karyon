/****************************************************************
 * open.c                                                       *
 *                                                              *
 *    open syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/ext2.h>
#include <fs/file.h>
#include <fs/path.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h> // debugging
#include <kernel/process.h>
#include <kernel/syscall.h>
#include <kernel/types.h>

/**
 * sys_open
 */

si32_t sys_open(uchar_t *path, ui32_t flags)
{
	ui32_t ext2_inum;
	struct ext2_file ext2_file;
	ret_t ret;
	ino_t inum;
	off_t off = 0; // needs to be initialized
	si32_t fildes;
	struct fildes *fd;

	#ifdef DEBUG
	printk("open(\"%s\", %x)\n", path, flags);
	#endif

	/** Check whether the path is not too long. **/

	if(strlen(path) > PATH_MAX_LEN)
	{
		goto fail;
	}

	/** Clean the path. **/

	strncpy(tmp_path, path, PATH_MAX_LEN + 1);

	if(path_clean(tmp_path) != OK)
	{
		goto fail;
	}

	/** Proceed differently according to the type of file. **/

	if(!strncmp(tmp_path, "/dev/tty", 9))
	{
		/** Terminal. **/

		inum = 1;
	}
	else if(!strncmp(tmp_path, "/dev/hd", 8))
	{
		/** Disk. **/

		inum = 2;
	}
	else
	{
		/** Regular files and directories. **/

		/** Try to convert the path into a file. **/

		ret = path_to_file(tmp_path, &ext2_inum, 0, 0);

		/** If no file corresponds to the path and the file is not
		    expected to be created, return an error. **/

		if((ret == -ENOENT && !(flags & O_CREAT))
		|| (ret != OK && ret != -ENOENT))
		{
			*current->perrno = (ui32_t)(-ret);
			goto fail;
		}

		/** Manage O_CREAT and O_EXCL. **/

		if(flags & O_CREAT)
		{
			if(ret != OK)
			{
				ret = path_create(tmp_path, 0, 0, &ext2_file);

				if(ret != OK)
				{
					*current->perrno = (ui32_t)(-ret);
					goto fail;
				}

				ext2_inum = ext2_file.inum;
			}
			else if(flags & O_EXCL)
			{
				goto fail;
			}
		}

		/** Look for the file in the file table. If the file was not
		    found, try to add it to the file table. **/

		if((ret = file_fetch(FS_EXT2,
		                     &ext2_inum,
		                     &ext2_file,
		                     0,
		                     &inum)) != OK)
		{
			goto fail;
		}

		if(!inum)
		{
			inum = file_alloc();

			if(!inum)
			{
				goto fail;
			}

			file_tab[inum - 1].fs = FS_EXT2;
			file_tab[inum - 1].data.ext2_file = ext2_file;
		}

		/** Note: these flags must be handled after locating the file
		    in or adding the file to the file table. **/

		/** Manage O_TRUNC. **/

		if(flags & O_TRUNC)
		{
			#ifdef DEBUG
			printk("truncate %x to 0\n", ext2_file.inum);
			delay(50000);
			#endif

			if(ext2_truncate(&file_tab[inum - 1].data.ext2_file,
			                 0) != OK)
			{
				goto fail;
			}
		}
	
		/** Manage O_APPEND. **/

		if(flags & O_APPEND)
		{
			off = (off_t)file_tab[inum - 1]	
                                    .data.ext2_file.inode.i_size_low;
		}
	}

	/** Allocate a file descriptor. **/

	fildes = fildes_alloc(0);

	if(fildes == -1)
	{
		goto fail;
	}

	/** Initialize the file descriptor. **/

	fd = current->pfildes_tab[fildes];
	current->fildes_flags[fildes] = flags;
	fd->off = off;
	fd->inum = inum;

	/** Reference the file. **/

	file_ref(inum);

	return fildes;

	fail:
		return -1;
}
