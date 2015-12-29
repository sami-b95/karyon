/****************************************************************
 * sigact.c                                                     *
 *                                                              *
 *    sigaction syscall.                                        *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <kernel/panic.h> // debugging
#include <kernel/process.h>
#include <kernel/signal.h>

/**
 * sys_sigaction
 */

int sys_sigaction(ui32_t sig, struct sigaction *act, struct sigaction *oldact)
{
	if(sig == 0 || sig > 22 || sig == SIGKILL || sig == SIGSTOP)
	{
		return -1;
	}

	#ifdef DEBUG
	printk("sig = %x, sys_sigaction: act = %x, act->sa_handler = %x\n",
	       sig, (ui32_t)act, (ui32_t)act->sa_handler);
	#endif

	/*if(sig == SIGTSTP)
	{
		if(act)
		{
			printk("handler for SIGTSTP: %x\n", act->sa_handler);
		}

		delay(50000);
	}*/

	if(oldact)
	{
		*oldact = current->sigact[sig - 1];
	}

	if(act)
	{
		current->sigact[sig - 1] = *act;
	}

	return 0;
}
