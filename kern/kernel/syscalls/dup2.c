/****************************************************************
 * dup2.c                                                       *
 *                                                              *
 *    dup2 syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/printk.h> // debugging
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_dup2
 */

int sys_dup2(si32_t old_fildes, si32_t new_fildes)
{
	struct fildes *old_fd, *new_fd;
	ret_t err;

	#ifdef DEBUG
	printk("dup2(%x, %x)\n", old_fildes, new_fildes);
	#endif

	if(old_fildes == new_fildes)
	{
		return old_fildes;
	}

	/** Check whether the file descriptors are valid. **/

	if((err = fildes_check(old_fildes, &old_fd, 0, 1, FS_NOFS)) != OK
	|| (err = fildes_check(new_fildes, &new_fd, 0, 0, FS_NOFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	/** If the new file descriptor is used, close it. **/

	if(new_fd)
	{
		#ifdef DEBUG
		printk("dup2 unreferences inode %x\n", new_fd->inum);
		#endif
		fildes_unref(new_fildes);
	}

	/** Copy the file descriptor. **/

	//printk("dup2(%x, %x)\n", old_fildes, new_fildes);
	current->pfildes_tab[new_fildes] = current->pfildes_tab[old_fildes];
	current->fildes_flags[new_fildes] = current->fildes_flags[old_fildes]
	                                  & ~O_CLOEXEC;
	fildes_ref(old_fildes);

	return new_fildes;
}
