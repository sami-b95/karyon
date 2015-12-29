/****************************************************************
 * fcntl.c                                                      *
 *                                                              *
 *    fcntl syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/stdarg.h>
#include <kernel/types.h>

/**
 * sys_fcntl
 */

/** FIXME About duplicating: in this implementation, the new file descriptor
          is a copy of the original one, and not an alias. **/

si32_t sys_fcntl(si32_t fildes, ui32_t cmd, va_list ap)
{
	struct fildes *fd;
	si32_t ret;
	ret_t err;

	if((err = fildes_check(fildes, &fd, 0, 1, FS_NOFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	#ifdef DEBUG
	printk("fcntl(%x, %x, %x)\n", fildes, cmd, ap);
	#endif

	switch(cmd)
	{
		case F_DUPFD:
			{
				si32_t new_fildes
				 = fildes_alloc(va_arg(ap, si32_t));

				if(new_fildes != -1)
				{
					current->pfildes_tab[new_fildes]
					= current->pfildes_tab[fildes];
					current->fildes_flags[new_fildes]
					 = current->fildes_flags[fildes]
					 & ~O_CLOEXEC;
					fildes_ref(fildes);
				}

				ret = new_fildes;
			}
			break;

		case F_SETFD:
			{
				ui32_t set = va_arg(ap, ui32_t);

				if(set & 1)
				{
					current->fildes_flags[fildes]
					 |= O_CLOEXEC;
				}
				else
				{
					current->fildes_flags[fildes]
					 &= ~O_CLOEXEC;
				}

				ret = 0;
			}
			break;

		case F_GETFL:
			ret = current->fildes_flags[fildes];
			break;


		case F_SETFL:
			{
				ui32_t flags = va_arg(ap, ui32_t);

				current->fildes_flags[fildes]
				 &= ~(O_APPEND | O_NONBLOCK);
				current->fildes_flags[fildes]
				 |= (flags & (O_APPEND | O_NONBLOCK));

				ret = 0;
			}
			break;

		case F_SETLK:
			// FIXME
			break;

		default:
			#ifdef DEBUG
			printk("unimplemented command %x\n", cmd);
			#endif
			ret = -1;
			break;
	}

	#ifdef DEBUG
	printk("fcntl: ret = %x\n", ret);
	#endif

	return ret;
}
