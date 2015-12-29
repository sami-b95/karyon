/****************************************************************
 * sigpmask.c                                                   *
 *                                                              *
 *    sigprocmask syscall.                                      *
 *                                                              *
 ****************************************************************/

#include <kernel/process.h>
#include <kernel/signal.h>
#include <kernel/types.h>

/**
 * sys_sigprocmask
 */

int sys_sigprocmask(ui32_t how, sigset_t *set, sigset_t *old_set)
{
	/** If old_set is not null, store the old signal mask at the address
	    pointed by old_set. **/

	if(old_set)
	{
		*old_set = current->sigmask;
	}

	/** Modify the signal mask if parameter how is valid. **/

	switch(how)
	{
		case SIG_BLOCK:
			current->sigmask |= *set;
			break;

		case SIG_UNBLOCK:
			current->sigmask &= ~(*set);
			break;

		case SIG_SETMASK:
			current->sigmask = *set;
			break;

		default:
			return -1;
	}

	return 0;
}
